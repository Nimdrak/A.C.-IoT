/* Phy_topology.h */

#ifndef PHY_TOPOLOGY_H
#define PHY_TOPOLOGY_H

#include <vector>
#include <fstream>

using namespace std;

class S_Node;
class S_Edge;
class Phy_Topo;

class S_Node {
	public:
		int node_ID;
		int edge_number;	// number of edges

		S_Node(int node_number);
		~S_Node();
	
		void PrintNodeInfo();
};

class S_Edge {
	public:
		int edge_ID;
		int node1;	// a connected node (small number)
		int node2;	// another connected node (large number)

		double weight;	// bandwidth of the edge

		double provisioned_weight;

		/* basic information */
		int n_accepted_requests;	
		double filled_weight;
		double total_var;	
		double min_epsilon;

		// embedded requests information on the edge
		typedef struct {
			int embedded_request_ID;
			double mean;
			double var;
			double epsilon;
		} Embedded_Info;
		typedef vector <Embedded_Info> Embedded_Info_Vector;
		typedef Embedded_Info_Vector::iterator embedded_info_iter;
		Embedded_Info_Vector m_embedded_info_vector;

		S_Edge();
		S_Edge(int edge_number, int current_node, int adjacent_node, int capacity);	
		~S_Edge();

		void SaveEmbeddedInfo(int _id, double _mean, double _var, double _eps);
		void SaveProvisioningInfo(double _bw);	
		void UpdateEdgeInfo();	
		void PrintEdgeInfo(ostream& out_steream);

		double GetActualQoS();
};

class Phy_Topo {
	public:
		S_Node** nodes;	// nodes, array of pointers with S_Node* type
		S_Edge** edges;	// edges, array of pointers with S_Edge* type

		int total_nodes;
		int total_edges;

		int** edge_node_map;	// table to find the node number connected to each edges

		Phy_Topo(int snodenum, int sedgenum, int capa);
		~Phy_Topo();	

		void UpdateAllEdgeInfo();

		void PrintAllNodeInfo();
		void PrintAllEdgeInfo(ostream& out_stream);
};

#endif
