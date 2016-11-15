/* C_Phy_topology.h */

#ifndef C_PHY_TOPOLOGY_H
#define C_PHY_TOPOLOGY_H

#include <vector>
#include <fstream>

using namespace std;

class C_S_Node;
class C_S_Edge;
class C_Phy_Topo;

class C_S_Node {
	public:
		int node_ID;
		int edge_number;	// number of edges

		C_S_Node(int node_number);
		~C_S_Node();
	
		void PrintNodeInfo();
};

class C_S_Edge {
	public:
		int edge_ID;
		int node1;	// a connected node (small number)
		int node2;	// another connected node (large number)

		double weight;	// bandwidth of the edge

		double provisioned_weight;

		/* basic information */
		int n_accepted_requests;	
		double filled_weight;

		// embedded requests information on the edge
		typedef struct {
			int embedded_request_ID;
			double demand;
		} Embedded_Info;
		typedef vector <Embedded_Info> Embedded_Info_Vector;
		typedef Embedded_Info_Vector::iterator embedded_info_iter;
		Embedded_Info_Vector m_embedded_info_vector;

		C_S_Edge();
		C_S_Edge(int edge_number, int current_node, int adjacent_node, double capacity);	
		~C_S_Edge();

		void SaveEmbeddedInfo(int _id, double _demand);
		void SaveProvisioningInfo(double _bw);	
		void UpdateEdgeInfo();	
		void PrintEdgeInfo(ostream& out_stream);

		double GetActualQoS();
};

class C_Phy_Topo {
	public:
		C_S_Node** nodes;	// nodes, array of pointers with C_S_Node* type
		C_S_Edge** edges;	// edges, array of pointers with C_S_Edge* type

		int total_nodes;
		int total_edges;

		int** edge_node_map;	// table to find the node number connected to each edges

		C_Phy_Topo(int snodenum, int sedgenum, double capa);
		~C_Phy_Topo();	

		void UpdateAllEdgeInfo();

		void PrintAllNodeInfo();
		void PrintAllEdgeInfo(ostream& out_steream);
};

#endif
