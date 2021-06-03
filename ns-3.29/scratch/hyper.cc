#include "communication_model.h"
#include <tuple>
#include <set>

using std::tuple;
using std::make_tuple;
using std::make_pair;
using std::get;
using std::set;

//tree variables
int D; //log_B N
int C; //compaction factor
bool group;

map<int, set<int>> *peers_send;
map<int, set<int>> *peers_recv;
map<int, map<int, set<int>>> *peers_dest;

void compact_broadcast_messages(vector<message> *messages)
{
	for(int node = 0; node < N; node++)
	{
		for(vector<message>::iterator m_it = messages[node].begin(); m_it != messages[node].end() && (*m_it).broadcast; m_it++)
		{
			int peer = (*m_it).send_to;
			if(peer != -1)
			{
				unsigned int i = 1;
				for(vector<message>::iterator it = messages[peer].begin() + 1;  it != messages[peer].end() && (*it).broadcast && i < (unsigned int) C; i++)
				{
					m_it = messages[node].insert(m_it + 1, *it);
					messages[it->send_to][0].recv_from = node;
					it = messages[peer].erase(it);
				}
			}
		}
	}
}

void set_broadcast_messages(vector<message> *messages, int node, int phase_of_node)
{
	if(phase_of_node == D)
	{
		return;
	}

	for(int d = phase_of_node; d < D; d++)
	{
		if((node ^ (1 << d)) < N)
		{
			int peer = (node ^ (1 << d));
			messages[node].push_back(message{peer, -1, HMAC_SIZE, -1, -1, true});
			messages[peer].push_back(message{-1, node, HMAC_SIZE, -1, -1, true});
			set_broadcast_messages(messages, peer, d + 1);
		}
	}
}

void set_messages(vector<message> *messages)
{
	//broadcast stage
	set_broadcast_messages(messages, start_node, 0);

	//hypercube stage
	peers_send = new map<int, set<int>>[N]();

	for(int node = 0; node < N; node++)
	{
		for(int d = 0; d < D; d++)
		{
			int peer = node ^ (1 << d);
			if(peer < N)
			{
				peers_send[node][d].emplace(peer);
			}
			else if(node >= (1 << (D - 1))) //peer does not exist
			{
				assert((1 << D) !=  N);
				
                //node is in the incomplete dimension, node needs to sub for nonexistant peer
                int future_phase = d;
				int future_peer;
                do
                {
                    future_phase++;
                    future_peer = peer ^ (1 << future_phase);
                }
                while(future_phase < D && future_peer >= N);

                assert(future_phase < D && future_peer < N);
				peers_send[node][d].emplace(future_peer);
			}			
		}
	}

	if(C > 1) //compaction
	{
		//broadcast compaction
		compact_broadcast_messages(messages);

		//hypercube compaction
		map<int, set<int>> *compacted_peers_send = new map<int, set<int>>[N]();
		int compacted_D = ceil(D*1.0/C);

		for(int node = 0; node < N; node++)
		{
			int phase = 0;
			for(int cD = 0; cD < compacted_D; cD++)
			{
				vector<pair<int, int>> stack;
				stack.push_back(make_pair(node, phase));
				while(!stack.empty())
				{
					pair<int,int> peer_and_phase = stack.back();
					stack.pop_back();
					compacted_peers_send[node][cD].emplace(peer_and_phase.first);
					for(int d = peer_and_phase.second; d < min(phase + C, D); d++)
					{
						for(int peer : peers_send[peer_and_phase.first][d])
						{
							stack.push_back(make_pair(peer, d + 1));
						}
					}
				}
				
				compacted_peers_send[node][cD].erase(node);				
				phase += C;			
			}				
		}

		//free memory
		for(int node = 0; node < N; node++)
		{
			for(int d = 0; d < D; d++)
			{
				peers_send[node][d].clear();
			}	
			peers_send[node].clear();
		}
		delete[] peers_send;
		peers_send = compacted_peers_send;	

		D = compacted_D;
		cout << "D: " << D << endl;
	}
	
	//each message sent by each node goes to which nodes, necessary to calculate message sizes in uneven cubes, compacted phases etc.
	peers_dest = new map<int, map<int, set<int>>>[N]();
	for(int d = D-1; d >= 0; d--)
	{
		for(int node = 0; node < N; node++)
		{
			for(int peer : peers_send[node][d])
			{
				peers_dest[node][d][peer].emplace(peer);
				for(int d2 = D-1; d2 > d; d2--)
				{
					for(pair<int, set<int>> set : peers_dest[peer][d2])
					{
						peers_dest[node][d][peer].insert(set.second.begin(), set.second.end());
					}
				}
			}
		}
	}

	//check if each nodes message reaches every other node once and only once
	for(int node = 0; node < N; node++)
	{
		//cout << "node: " << node << endl;
		set<int> test;
		unsigned int counter = 0;
		for(int d = 0; d < D; d++)
		{
			//cout << "\td: " << d << endl;
			for(pair<int, set<int>> set : peers_dest[node][d])
			{
				//cout << "\t\t" << set.first << ": ";
				for(int i : set.second)
				{
					//cout << i << ",";
					test.insert(i);
					counter++;
				}
				//cout << endl;
			}
		}
		assert(counter == test.size() && test.size() == (unsigned int) (N-1));
	}

	//peers receive messages from which nodes
	peers_recv = new map<int, set<int>>[N]();
	for(int node = 0; node < N; node++)
	{
		for(int d = 0; d < D; d++)
		{
			for(int peer : peers_send[node][d])
			{
				peers_recv[peer][d].emplace(node);
			}
		}
	}

	//populate the message table
	for(int node = 0; node < N; node++)
	{
		for(int d = 0; d < D; d++)
		{
			for(int peer : peers_send[node][d]) //send
			{
				messages[node].push_back(message{peer, -1, -1, d, 0, false});
			}
			for(int peer : peers_recv[node][d]) //recv
			{
				messages[node].push_back(message{-1, peer, -1, d, 0, false});
			}
		}
	}

	//set message sizes
	for(int node = 0; node < N; node++)
	{
		for(unsigned int msg = 0; msg < messages[node].size(); msg++)
		{
			if(messages[node][msg].broadcast)
			{
				continue;
			}
			if(messages[node][msg].send_to != -1)
			{
				if(full_msg_sizes)
				{
					messages[node][msg].size = HMAC_SIZE*N;
				}
				else
				{
					messages[node][msg].size = HMAC_SIZE*(peers_dest[node][messages[node][msg].phase][messages[node][msg].send_to].size());
				}
			}
			else if(messages[node][msg].recv_from != -1)
			{
				if(full_msg_sizes)
				{
					messages[node][msg].size = HMAC_SIZE*N;
				}
				else
				{
					messages[node][msg].size = HMAC_SIZE*(peers_dest[messages[node][msg].recv_from][messages[node][msg].phase][node].size());
				}
			}
		}
	}

	//free memory
	for(int node = 0; node < N; node++)
	{
		for(int d = 0; d < D; d++)
		{
			peers_send[node][d].clear();
			peers_recv[node][d].clear();
			for(pair<int, set<int>> p : peers_dest[node][d])
			{
				p.second.clear();
			}
			peers_dest[node][d].clear();
		}
		peers_send[node].clear();
		peers_recv[node].clear();
		peers_dest[node].clear();
	}
	delete[] peers_send;
	delete[] peers_recv;
	delete[] peers_dest;
}

void set_experiment_name()
{
	if(topology.compare("star") == 0)
	{
		experiment = "hyper";
	}
	else if(topology.compare("star_as") == 0)
	{
		experiment = "hyper_as";
	}
	else if(topology.compare("brite") == 0)
	{
		experiment = "hyper_brite";
	}
	else
	{
		assert(false);
	}

	if(C > 1)
	{
		experiment += "_f" + std::to_string(C);
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
	C = 1;

	CommandLine cmd;
	cmd.AddValue("C", "compaction factor", C);
	cmd.AddValue("group", "group", group);
	parse_default_arguments(cmd, argc, argv);

	set_experiment_name();
	D = ceil(log2(N));
	cout << "N: " << N << " D: " << D <<  " C: " << C << " group: " << group << endl;
	assert(C <= D);

	start_node = 0;
	generate_topology(group);
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

