#include <iostream>
#include <cassert>
#include <string>
#include <map>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/brite-module.h"
#include "ns3/ipv4-nix-vector-helper.h"

using namespace ns3;
using std::string;
using std::map;
using std::vector;
using std::cout;
using std::endl;
using std::min;
using std::pair;

NS_LOG_COMPONENT_DEFINE("CommunicationModel");

typedef map<Ptr<Socket>, vector<int>> data;

struct message
{
    int send_to;
    int recv_from;
    int size;
    int phase;
    int c_phase;
    bool broadcast;
};

//dimension variables
int N;
int no_AS;

//optimization variables
bool full_msg_sizes;

//send message size variables
const int HMAC_SIZE = 32;

//TCP segment variables
const int MTU = 1500;
const int HEADERS = 32 + 32;
const int TCP_PAYLOAD = MTU - HEADERS;
uint8_t dummy_data[TCP_PAYLOAD]; //data to write into packets

//node IP + AS assignment variables
map<int, Ipv4Address>   node_ips;
map<Ipv4Address, int>   node_ids;
map<int,int>    		AS_leads;
vector<int>				*AS_members;

//TCP data variables
data rcvd_data;
data sent_data;
data to_send;

//TCP sockets variables
map<int, Ptr<Socket>>   *sockets;

//experiment variables
int current_run;
int no_runs;

//communication logic variables
vector<message> *messages;
vector<int>		*node_buffers;
int             *current_msg;
int             no_rcvd_proposal;
int             no_rcvd_hash;

//logging variables
std::ofstream       results;
bool                verbose;
bool                log_experiment;
bool                monitor_flow;
FlowMonitorHelper   fmh;
Ptr<FlowMonitor>    monitor;

//ns3 variables
PointToPointHelper point_to_point;
InternetStackHelper internet;
Ipv4AddressHelper ipv4;
NodeContainer nodes;

string results_dir;
string experiment;
string topology;
int	  start_node;

void connect_sockets(NodeContainer nodes);
void set_callbacks();
void write(Ptr<Socket> socket, uint32_t available);
void send(int node);
void recv(Ptr<Socket> socket);
void generate_brite_topology(bool group);
void generate_AS_star_topology(bool group);
void generate_star_topology();

void allocate()
{
	AS_members = new vector<int>[no_AS]();
	messages = new vector<message>[N]();
	node_buffers = new vector<int>[N]();
	current_msg = new int[N]();

	sockets = new map<int,Ptr<Socket>>[N]();
}

void reset_experiment()
{
	no_rcvd_hash = 0;
	no_rcvd_proposal = 1;    

	for (int i = 0; i < N; i++)
	{
		node_buffers[i].clear();
		current_msg[i] = 0;
	}

	for(int i = 0; i < N; i++)
	{
		for(pair<int, Ptr<Socket>> map_entry : sockets[i])
		{
			if(i == map_entry.first)
			{
				continue;
			}

			rcvd_data[map_entry.second].clear();
			sent_data[map_entry.second].clear();
			to_send[map_entry.second].clear();
		}		
	}
}

void print_messages(vector<message> *messages)
{
	cout << "messages..." << endl;

	for(int node = 0; node < N; node++)
	{
		cout << node << ": ";
		for(unsigned int i = 0; i < messages[node].size(); i++)
		{
			cout << "(" << messages[node][i].send_to << "," << messages[node][i].recv_from << "," <<
							messages[node][i].phase << "," << messages[node][i].size << ");";
		}
		cout << endl;
	}
}	

void setup_experiment()
{
	if(verbose)
	{
		cout << "setup" << endl;
	}

	no_rcvd_proposal = 1;
	no_rcvd_hash = 0;

	for(int i = 0; i < TCP_PAYLOAD; i++)
	{
		dummy_data[i] = toascii (97 + i % 26);
	}
}

void initialize_variables()
{
	srand(time(NULL));
	N = 8;
	no_AS = 0;
	no_runs = 1;
	verbose = false;
	monitor_flow = false;
	topology = "star";
	results_dir = "";
}

void parse_default_arguments(CommandLine &cmd, int argc, char *argv[])
{
	cmd.AddValue("N", "number of nodes", N);
	cmd.AddValue("AS", "number of ASes", no_AS);
	cmd.AddValue("verbose", "print detailed info", verbose);
	cmd.AddValue("monitor_flow", "monitor flows", monitor_flow);
	cmd.AddValue("topology", "topology", topology);
	cmd.AddValue("no_runs", "number of runs", no_runs);
	cmd.AddValue("results", "directory for the results", results_dir);
	cmd.AddValue("full_msg_sizes", "turns off the optimization for message sizes", full_msg_sizes);
    cmd.Parse(argc, argv);
    
    if(no_AS == 0)
	{
		no_AS = ceil(N/128.0);
	}

    allocate();
}

void generate_topology(bool group)
{
	if(topology.compare("star") == 0)
	{
		generate_star_topology();
	}
	else if(topology.compare("star_as") == 0)
	{
		generate_AS_star_topology(group);
	}
	else if(topology.compare("brite") == 0)
	{
		generate_brite_topology(group);
	}
	else
	{
		assert(false);
	}
}

void run_experiment()
{
	if(results_dir.compare("") != 0)
	{
		string dir = results_dir + "/" + experiment + "/" + std::to_string(N) + "/data";
		results.open(dir, std::ios_base::app);
		results << endl << "RUN: " << endl;
	}	

	bool trace = false;
	if(trace)
	{
		AsciiTraceHelper ascii;
		Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (experiment + ".tr");
		point_to_point.EnableAsciiAll (stream);
		internet.EnableAsciiIpv4All (stream);
		Ipv4GlobalRoutingHelper g;
		Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (experiment + ".routes", std::ios::out);
		g.PrintRoutingTableAllAt(Seconds(10), routingStream);
	}

	setup_experiment();

	LogComponentEnableAll(LOG_PREFIX_TIME);
	LogComponentEnable("CommunicationModel", LOG_LEVEL_INFO);

	Simulator::ScheduleNow(&connect_sockets, nodes);
	Simulator::Run();
	Simulator::ScheduleNow(&set_callbacks);
	Simulator::Run();
	
	if(monitor_flow)
	{
		monitor = fmh.InstallAll(); 
	}
	
	current_run = 0;
	Simulator::ScheduleNow(&send, start_node);
	Simulator::Run();
		
	if(monitor_flow)
	{
		monitor->SerializeToXmlFile(experiment + ".xml", true, true);
		monitor->CheckForLostPackets (); 
		Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (fmh.GetClassifier ());
		std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
		{
			Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);

			NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr " << node_ids[t.sourceAddress] << "@" <<
			t.sourceAddress << " Dst Addr " << node_ids[t.destinationAddress] << "@" << t.destinationAddress);
			NS_LOG_UNCOND("Tx Packets = " << iter->second.txPackets);
			NS_LOG_UNCOND("Rx Packets = " << iter->second.rxPackets);
			NS_LOG_UNCOND("Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps");
			if(iter->second.txPackets != iter->second.rxPackets)
			{
				NS_LOG_UNCOND("LOST PACKETS: " << (iter->second.txPackets - iter->second.rxPackets));
			}
		}
	}

	Simulator::Destroy();
}

/*
*	connecting sockets functions
*/

Ptr<Socket> get_socket(Ptr<Node> node)
{
	Ptr<Socket> socket = Socket::CreateSocket(node, TypeId::LookupByName("ns3::TcpSocketFactory"));
	socket->SetAttribute ("SegmentSize", UintegerValue (TCP_PAYLOAD));
	return socket;
}

void socket_accept(Ptr<Socket> socket, const Address &address)
{
	Address node_address;
	socket->GetSockName(node_address);
	int node = node_ids[InetSocketAddress::ConvertFrom(node_address).GetIpv4()];
	int peer = node_ids[InetSocketAddress::ConvertFrom(address).GetIpv4()];

	sockets[node][peer] = socket;
}

void connect_sockets(NodeContainer nodes)
{
	NS_LOG_INFO("creating listening sockets for nodes");
	for (int n = 0; n < N; n++)
	{
		sockets[n][n] = get_socket(nodes.Get(n));
		sockets[n][n]->Bind(InetSocketAddress(node_ips[n], 80));
		sockets[n][n]->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address &>(), MakeCallback(&socket_accept));
		sockets[n][n]->Listen();
	}

	NS_LOG_INFO("connect each socket to its peers");
	for (int n = 0; n < N; n++)
	{
		for(unsigned int i = 0; i < messages[n].size(); i++)
		{
			int send_to =  messages[n][i].send_to;
			int recv_from = messages[n][i].recv_from;
			if(send_to != -1 && send_to > n &&(sockets[n].find(send_to) == sockets[n].end()))
			{
				sockets[n][send_to] = get_socket(nodes.Get(n));
				sockets[n][send_to]->Connect(InetSocketAddress(node_ips[send_to], 80));
			}
			if(recv_from != -1 && recv_from > n &&(sockets[n].find(recv_from) == sockets[n].end()))
			{
				sockets[n][recv_from] = get_socket(nodes.Get(n));
				sockets[n][recv_from]->Connect(InetSocketAddress(node_ips[recv_from], 80));
			}
		}
	}
}

void set_callbacks()
{
	NS_LOG_INFO("setting regular callback functions for sockets ");

	for(int node = 0; node < N; node++)
	{
		for(pair<int, Ptr<Socket>> socket : sockets[node])
		{
			socket.second->SetSendCallback(MakeCallback(&write));
			socket.second->SetRecvCallback(MakeCallback(&recv));
			sent_data[socket.second];
			rcvd_data[socket.second];
			to_send[socket.second];
		}
	}
}

/*
*   read write functions
*/ 

int get_message_index(Ptr<Socket> write_socket, Ptr<Socket> read_socket, bool write)
{
	if(write)
	{
		for(unsigned int i = 0 ; i < to_send[write_socket].size(); i++)
		{
			if(sent_data[write_socket][i] < to_send[write_socket][i])
			{
				return i;
			}
		}
	}
	else
	{
		for(unsigned int i = 0; i < to_send[write_socket].size(); i++)
		{
			if(rcvd_data[read_socket][i] < to_send[write_socket][i])
			{
				return i;
			}
		}
	}
	return -1;
}

void write(Ptr<Socket> socket, uint32_t available)
{
	int msg = get_message_index(socket, NULL, true);	
	if(msg == -1)
	{
		return;
	}

	while(sent_data[socket][msg] < to_send[socket][msg] && socket->GetTxAvailable() > 0)
	{
		int left = to_send[socket][msg] - sent_data[socket][msg];
		int offset = sent_data[socket][msg] % TCP_PAYLOAD;
		int to_write = min(min(TCP_PAYLOAD - offset, left), (int) socket->GetTxAvailable());
		int s = socket->Send(dummy_data + offset, to_write, 0);
		if(s < 0)
		{
			return;
		}
		sent_data[socket][msg] = sent_data[socket][msg] + s;
	}
}

void send(int node)
{
	int current = current_msg[node];
	
	if(current == (int) messages[node].size())
	{
		return;
	}

	if(node == start_node && current == 0)
	{
		NS_LOG_INFO("RUN: " << current_run);
		if(results_dir.compare("") != 0)
		{
			results << Simulator::Now() << ",";		
		}
		log_experiment = (current_run == no_runs - 1);
		if(log_experiment)
		{
			NS_LOG_INFO("START OF EXPERIMENT");
		}
	}

	int peer = messages[node][current].send_to;
	if(peer == -1)
	{
		return;
	}

	rcvd_data[sockets[peer][node]].push_back(0);
	sent_data[sockets[node][peer]].push_back(0);
	to_send[sockets[node][peer]].push_back(messages[node][current].size);

	write(sockets[node][peer],sockets[node][peer]->GetTxAvailable());
		
	if(log_experiment)
	{
		NS_LOG_INFO("node " << node << " sent " << sent_data[sockets[node][peer]].back() << " bytes to node " << peer << "@" << node_ips[peer]);
	}

	if(messages[node][current_msg[node]].recv_from == -1)
	{
		current_msg[node]++;
		send(node);
	}
}

void handle_message(int node, int peer)
{
	if(current_msg[node] == 0)
	{
		no_rcvd_proposal++;
		if(no_rcvd_proposal == N)
		{
			if(log_experiment)
			{
				NS_LOG_INFO("LOG TIMESTAMP: all nodes received proposal");
			}
			if(results_dir.compare("") != 0)
			{
				results << Simulator::Now() << ",";	
			}
		}
	}

	current_msg[node]++;
	send(node);
	
	if(current_msg[node] == (int) messages[node].size())
	{
		assert(node_buffers[node].size() == 0);
		if(log_experiment)
		{
			NS_LOG_INFO("DONE: " << node);
		}
		no_rcvd_hash++;
		if(no_rcvd_hash == N)
		{
			if(log_experiment)
			{
				NS_LOG_INFO("LOG TIMESTAMP: all nodes are done");
			}

			if(results_dir.compare("") != 0)
			{
				results << Simulator::Now() << ",";		
			}
			current_run++;
			if(current_run < no_runs)
			{
				if(results_dir.compare("") != 0)
				{
					results << endl;
				}
				reset_experiment();
				send(start_node);
			}
			else if(monitor_flow)
			{
				Simulator::Stop();
			}
		}
	}
	
}

void check_buffer(int node)
{
	bool changed;
	vector<int> unbuffered;
	do
	{
		changed = false;
		int peer = messages[node][current_msg[node]].recv_from;
		vector<int>::iterator it = node_buffers[node].begin();
		while(it != node_buffers[node].end())
		{
			if (*it == peer)
			{
				it = node_buffers[node].erase(it);
				unbuffered.push_back(peer);
				handle_message(node, peer);
				changed = true;
				break;
			}
			else
			{
				++it;
			}
		}
	}
	while(changed);

	if(!unbuffered.empty())
	{
		string s = "";
		for(int i : unbuffered)
		{
			s += std::to_string(i) + ",";
		}
		if(log_experiment)
			NS_LOG_INFO("node " << node << " unbuffered message from " << s);
	}
}

void recv(Ptr<Socket> socket)
{
	Address node_address;
	socket->GetSockName(node_address);
	int node = node_ids[InetSocketAddress::ConvertFrom(node_address).GetIpv4()];

	Address from;
	Ptr<Packet> packet = socket->RecvFrom(from);
	packet->RemoveAllPacketTags();
	packet->RemoveAllByteTags();
	InetSocketAddress address = InetSocketAddress::ConvertFrom(from);
	int peer = node_ids[address.GetIpv4()];

	int msg_index = get_message_index(sockets[peer][node], socket, false);

	rcvd_data[socket][msg_index] += packet->GetSize();

	int msg_size = to_send[sockets[peer][node]][msg_index];

	if (rcvd_data[socket][msg_index] < msg_size)
	{
		return;
	}

	int total_size = rcvd_data[socket][msg_index];
	if(log_experiment)
	{
		NS_LOG_INFO("node " << node << " received " << total_size  << " bytes from node " << peer << "@" << address.GetIpv4());
	}

	while (total_size >= msg_size)
	{
		total_size -= msg_size;

		assert(messages[node][current_msg[node]].recv_from != -1);
		if(messages[node][current_msg[node]].recv_from != peer)
		{
			node_buffers[node].push_back(peer);
			if(log_experiment)
			{
				NS_LOG_INFO("node " << node << " buffered " << msg_size << " bytes from node " << peer << "@" << address.GetIpv4());
			}
		}
		else
		{
			handle_message(node, peer);
		}

		if(total_size > 0)
		{
			rcvd_data[socket][msg_index] -= total_size;
			msg_index = get_message_index(sockets[peer][node], socket, false);
			msg_size = to_send[sockets[peer][node]][msg_index];
			rcvd_data[socket][msg_index] = total_size;
		}        
	}

	check_buffer(node);
}

/*
* topology generation functions
*/

void generate_brite_topology(bool group)
{
	Ipv4NixVectorHelper nixRouting;
	Ipv4StaticRoutingHelper staticRouting;
	Ipv4ListRoutingHelper list;
	list.Add(staticRouting, 0);
	list.Add(nixRouting, 10);
	internet.SetRoutingHelper(list);

	ipv4.SetBase ("10.1.1.0", "255.255.255.0");

	std::string filename = "../BRITE/conf_files/TDBW" + std::to_string(no_AS) + ".conf";
	cout << "filename: " << filename << endl;

	BriteTopologyHelper bth(filename);
	cout << "imported file..." << endl;
	bth.AssignStreams(3);
	bth.BuildBriteTopology(internet);
	bth.AssignIpv4Addresses(ipv4);

	assert(no_AS == (int) bth.GetNAs());
	for (int i = 0; i < no_AS; i++)
	{
		cout << "AS" << i << " : " << bth.GetNLeafNodesForAs(i) << " nodes" << endl;
	}

	int nodes_per_AS = ceil(N/no_AS);
	nodes.Create(nodes_per_AS*no_AS);
	internet.Install(nodes);

	//divide node ids randomly
	vector<int> node_id_set;
	for(unsigned int i = 0; i < nodes.GetN(); i++)
	{
		node_id_set.push_back(i);
	}
	if(!group)
	{
		std::random_shuffle(node_id_set.begin(), node_id_set.end());
	}

	point_to_point.SetDeviceAttribute("DataRate", StringValue("10Gbps"));
	point_to_point.SetChannelAttribute("Delay", StringValue("1ms"));

	for(int a = 0; a < no_AS; a++)
	{
		bool first_member = true;
		for (int n = 0; n < nodes_per_AS; n++)
		{
			ipv4.NewNetwork();
			int node_id = node_id_set.at(a*nodes_per_AS + n);
			NetDeviceContainer devices = point_to_point.Install(bth.GetLeafNodeForAs(a, n), nodes.Get(node_id));
			ipv4.Assign(devices.Get(0));
			node_ips[node_id] = ipv4.Assign(devices.Get(1)).GetAddress(0);
			node_ids[node_ips[node_id]] = node_id;
			if(first_member || (node_id == start_node))
			{
				AS_leads[a] = node_id;
				first_member = false;
			}
			AS_members[a].push_back(node_id);
			if(verbose)
				cout << "node " << node_id << ", id: " << nodes.Get(node_id)->GetId() << " assigned " << node_ips[node_id] 
						<< " in AS " << a << endl;
		}
	}
}

void generate_AS_star_topology(bool group)
{
	Ipv4NixVectorHelper nixRouting;
	Ipv4StaticRoutingHelper staticRouting;
	Ipv4ListRoutingHelper list;
	list.Add(staticRouting, 0);
	list.Add(nixRouting, 10);
	internet.SetRoutingHelper(list);

	ipv4.SetBase ("10.1.1.0", "255.255.255.0");

	int nodes_per_AS = ceil(N/no_AS);
	NodeContainer routers;
	routers.Create(no_AS + 1);
	nodes.Create(nodes_per_AS*no_AS);

	internet.Install(routers);
	internet.Install(nodes);

	//first build the ASes
	point_to_point.SetDeviceAttribute("DataRate", StringValue("20Gbps"));
	point_to_point.SetChannelAttribute("Delay", StringValue("1ms"));

	//divide node ids randomly
	vector<int> node_id_set;
	for(unsigned int i = 0; i < nodes.GetN(); i++)
	{
		node_id_set.push_back(i);
	}
	if(!group)
	{
		std::random_shuffle(node_id_set.begin(), node_id_set.end());
	}

	for(int a = 0; a < no_AS; a++)
	{
		bool first_member = true;
		cout << "router id: " << routers.Get(a)->GetId() << endl;
		for (int n = 0; n < nodes_per_AS; n++)
		{
			int node_id = node_id_set.at(a*nodes_per_AS + n);
			NetDeviceContainer devices = point_to_point.Install(routers.Get(a), nodes.Get(node_id));
			ipv4.Assign(devices.Get(0));
			node_ips[node_id] = ipv4.Assign(devices.Get(1)).GetAddress(0);
			node_ids[node_ips[node_id]] = node_id;
			if(first_member || (node_id == start_node))
			{
				AS_leads[a] = node_id;
				first_member = false;
			}
			AS_members[a].push_back(node_id);
			if(verbose)
				cout << "node " << node_id << ", id: " << nodes.Get(node_id)->GetId() << " assigned " << node_ips[node_id] 
						<< " in AS " << a << endl;
			ipv4.NewNetwork();
		}
	}

	//connect ASes
	point_to_point.SetDeviceAttribute("DataRate", StringValue("20Gbps"));
	point_to_point.SetChannelAttribute("Delay", StringValue("100ms"));

	cout << "core router id: " << routers.Get(no_AS)->GetId() << endl;
	for(int a = 0; a < no_AS; a++)
	{
		NetDeviceContainer devices = point_to_point.Install(routers.Get(no_AS), routers.Get(a));
		ipv4.Assign(devices.Get(0));
		ipv4.Assign(devices.Get(1));
		ipv4.NewNetwork();
	}
}

void generate_star_topology()
{
	Ipv4NixVectorHelper nixRouting;
	Ipv4StaticRoutingHelper staticRouting;
	Ipv4ListRoutingHelper list;
	list.Add(staticRouting, 0);
	list.Add(nixRouting, 10);
	internet.SetRoutingHelper(list);

	ipv4.SetBase ("10.1.1.0", "255.255.255.0");

	point_to_point.SetDeviceAttribute("DataRate", StringValue("20Gbps"));
	point_to_point.SetChannelAttribute("Delay", StringValue("100ms"));

	nodes.Create(N);    
	NodeContainer router;
	router.Create(1);
	internet.Install(nodes);
	internet.Install(router);
	
	cout << "router id: " << router.Get(0)->GetId() << endl;

	for(int i = 0; i < N; i++)
	{
		NetDeviceContainer devices = point_to_point.Install(router.Get(0), nodes.Get(i));
		ipv4.Assign(devices.Get(0));
		node_ips[i] = ipv4.Assign(devices.Get(1)).GetAddress(0);
		node_ids[node_ips[i]] = i;
		if(verbose)
			cout << "node " << i << ", id: " << nodes.Get(i)->GetId() << " assigned " << node_ips[i] << endl;
		ipv4.NewNetwork();
	}
}
