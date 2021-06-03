#include "communication_model.h"

//tree variables
int D; //log_B N
int B; //branching factor
bool group;
bool bcast_tree;

void set_bcast_tree_messages(vector<message> *messages)
{
	//broadcast
	for(int as = 0; as < no_AS; as++)
	{
		if(AS_leads[as] != start_node)
		{
			messages[start_node].push_back(message{AS_leads[as], -1, HMAC_SIZE, 0, -1, true});
			messages[AS_leads[as]].push_back(message{-1, start_node, HMAC_SIZE, 0, -1, true});
		}
	}
	for(int as = 0; as < no_AS; as++)
	{
		for(int node : AS_members[as])
		{
			if(node != AS_leads[as])
			{
				messages[AS_leads[as]].push_back(message{node, -1, HMAC_SIZE, 0, -1, true});
				messages[node].push_back(message{-1, AS_leads[as], HMAC_SIZE, 0, -1, true});
			}
		}
	}

	//transmit
	for(int as = 0; as < no_AS; as++)
	{
		for(int node : AS_members[as])
		{
			if(node != AS_leads[as])
			{
				messages[node].push_back(message{AS_leads[as], -1, N*HMAC_SIZE, 1, -1, false});
				messages[AS_leads[as]].push_back(message{-1, node, N*HMAC_SIZE, 0, -1, true});
			}
		}

		if(AS_leads[as] != start_node)
		{
			messages[AS_leads[as]].push_back(message{start_node, -1, N*HMAC_SIZE, 0, -1, true});
			messages[start_node].push_back(message{-1, AS_leads[as],  N*HMAC_SIZE, 0, -1, true});
		}
	}

	//broadcast
	for(int as = 0; as < no_AS; as++)
	{
		int AS_size = (int) AS_members[as].size();
		if(AS_leads[as] != start_node)
		{
			messages[start_node].push_back(message{AS_leads[as], -1, AS_size*HMAC_SIZE, 0, -1, true});
			messages[AS_leads[as]].push_back(message{-1, start_node, AS_size*HMAC_SIZE, 0, -1, true});
		}
	}	
	for(int as = 0; as < no_AS; as++)
	{
		for(int node : AS_members[as])
		{
			if(node != AS_leads[as])
			{
				messages[AS_leads[as]].push_back(message{node, -1, HMAC_SIZE, 0, -1, true});
				messages[node].push_back(message{-1, AS_leads[as], HMAC_SIZE, 0, -1, true});
			}
		}
	}
}

void set_messages(vector<message> *messages)
{
	//broadcast stage
	for(int node = 0; node < pow(B,D); node++)
	{
		for(int b = 1; b <= B; b++)
		{
			int peer = B*node + b;
			if(peer >= N)
			{
				break;
			}
			messages[node].push_back(message{peer, -1, HMAC_SIZE, 0, -1, true});
			messages[peer].push_back(message{-1, node, HMAC_SIZE, 0, -1, true});
		}
	}

	//transmit stage + message size calculation for next stage
	map<int,int> down;
	for(int node = N - 1; node > 0; node--)
	{
		int peer = floor((node-1)*1.0/B);

		messages[node].push_back(message{peer, -1, N*HMAC_SIZE, 1, -1, false});
		messages[peer].push_back(message{-1, node, N*HMAC_SIZE, 1, -1, false});

		down[node] = 1;
		for(int b = 1; b <= B; b++)
		{
			int peer = B*node + b;
			if(peer >= N)
			{
				break;
			}
			down[node] = down[node] + down[peer];
		}
	}

	//broadcast stage
	for(int node = 0; node < pow(B,D); node++)
	{
		for(int b = 1; b <= B; b++)
		{
			int peer = B*node + b;
			if(peer >= N)
			{
				continue;
			}

			if(full_msg_sizes)
			{
				messages[node].push_back(message{peer, -1, HMAC_SIZE*N, 2, -1, false});
				messages[peer].push_back(message{-1, node, HMAC_SIZE*N, 2, -1, false});
			}
			else
			{
				messages[node].push_back(message{peer, -1, HMAC_SIZE*down[peer], 2, -1, false});
				messages[peer].push_back(message{-1, node, HMAC_SIZE*down[peer], 2, -1, false});
			}
		}
	}
}

void set_experiment_name()
{
	experiment = "";

	if(bcast_tree)
	{
		experiment += "bcast_";
	}

	if(topology.compare("star") == 0)
	{
		experiment += "tree";
	}
	else if(topology.compare("star_as") == 0)
	{
		experiment += "tree_as";
	}
	else if(topology.compare("brite") == 0)
	{
		experiment += "tree_brite";
	}
	else
	{
		assert(false);
	}

	if(B != 2)
	{
		experiment += "_b" + std::to_string(B);
	}
	if(group)
	{
		experiment += "_g";
	}
	if(full_msg_sizes)
	{
		experiment += "_full";
	}
}


void run(int argc, char *argv[])
{
	initialize_variables();
	group = false;
	bcast_tree = false;
	B = 2;

	CommandLine cmd;
	cmd.AddValue("B", "branching factor", B);
	cmd.AddValue("group", "group", group);
	cmd.AddValue("bcast", "broadcast intra and inter as", bcast_tree);
	parse_default_arguments(cmd, argc, argv);

	set_experiment_name();
	D = ceil(log(N)/log(B));
	cout << "N: " << N << " B: " << B << " D: " << D << " group: " << group << endl;

	start_node = 0;
	generate_topology(group);
	if(bcast_tree)
	{
		set_bcast_tree_messages(messages);;
	}
	else
	{
		set_messages(messages);
	}
	
	if(verbose)
	{
		print_messages(messages);
	}

	run_experiment();
}



int main(int argc, char *argv[])
{
	run(argc, argv);
}

