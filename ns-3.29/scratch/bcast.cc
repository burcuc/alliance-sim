#include "communication_model.h"


void set_messages(vector<message> *messages)
{
	//broadcast stage
	for(int peer = 0; peer < N; peer++)
	{
        if(peer == start_node)
        {
            continue;
        }
        messages[start_node].push_back(message{peer, -1, HMAC_SIZE, 0, -1, true});
		messages[peer].push_back(message{-1, start_node, HMAC_SIZE, 0, -1, true});
	}

	//transmit stage
	for(int node = 0; node < N; node++)
	{
        if(node == start_node)
        {
            continue;
        }
		messages[node].push_back(message{start_node, -1, N*HMAC_SIZE, 1, -1, false});
		messages[start_node].push_back(message{-1, node, N*HMAC_SIZE, 1, -1, false});
	}

	//broadcast stage
    for(int peer = 0; peer < N; peer++)
	{
        if(peer == start_node)
        {
            continue;
        }
        messages[start_node].push_back(message{peer, -1, HMAC_SIZE, 0, -1, true});
		messages[peer].push_back(message{-1, start_node, HMAC_SIZE, 0, -1, true});
	}
}

void set_experiment_name()
{
	if(topology.compare("star") == 0)
	{
		experiment = "bcast";
	}
	else if(topology.compare("star_as") == 0)
	{
		experiment = "bcast_as";
	}
	else if(topology.compare("brite") == 0)
	{
		experiment = "bcast_brite";
	}
	else
	{
		assert(false);
	}
}


void run(int argc, char *argv[])
{
	initialize_variables();

	CommandLine cmd;
	parse_default_arguments(cmd, argc, argv);

	set_experiment_name();
	cout << "N: " << N << endl;

	start_node = 0;
	generate_topology(false);
	set_messages(messages);
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

