/* Phy_Topology.h */

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
		int Node_ID;	// node identification
		int Connected_Edge_Number; 

		S_Node();
		~S_Node();

		void PrintNodeInformation();
};

class S_Edge {
	public:
		int Edge_ID;
		int Connected_Node1;
		int Connected_Node2;	// connected nodes, Node1 has smaller Node_ID than Node2;

		double Bandwidth;	// Bandwidth of the edge
		double Filled_Bandwidth;

		/* basic request information on this edge */
		typedef struct {
			int Embedded_Request_ID;
			double Mean;
			double Var;
			double Epsilon;
		} Embedded_Request_Information;
		typedef vector <Embedded_Request_Information> Embedded_Request_Information_Vector;
		typedef Embedded_Request_Information_Vector::iterator Embedded_Request_Information_Iter;
		Embedded_Request_Information_Iter m_embedded_request_info_vector;

		int n_Accepted_requests;
		double Total_Var;
		double Min_Epsilon;

		S_Edge();
		~S_Edge();

		void UpdateEdgeInformation();
		void PrintEdgeInformation();

		double GetActualEpsilon();
};

class Phy_Topo {
	public:
		S_Node** Nodes;	// nodes: array of pointers to indicate S_Node* type
		S_Edge** Edges;	// edges: array of pointers to indicate S_Edge* type

		int Total_Node_Number;
		int Total_Edge_Number;

		Phy_Topo();
		~Phy_Topo();

		void UpdateAllEdgeInformation();

		void PrintAllEdgeInformation(ofstream& out_steream);
};

#endif
