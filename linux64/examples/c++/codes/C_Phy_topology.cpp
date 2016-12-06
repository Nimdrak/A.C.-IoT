#include <fstream>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <list>

#include "C_Phy_topology.h"

#define MAX_SIZE 100
#define DELIM "\t\n"

char c_inputString[MAX_SIZE];

using namespace std;

/* C_S_Node Class */
C_S_Node::C_S_Node(int node_number) {
	node_ID = node_number;
	edge_number = 0;
}

C_S_Node::~C_S_Node() {

}

void C_S_Node::PrintNodeInfo() {

}

/* C_S_Edge Class */
C_S_Edge::C_S_Edge() {

}

C_S_Edge::C_S_Edge(int edge_number, int current_node, int adjacent_node, double capacity) {
	edge_ID = edge_number;
	node1 = current_node;
	node2 = adjacent_node;
	weight = capacity;

	provisioned_weight = 0.0;

	n_accepted_requests = 0;
	filled_weight = 0.0;
}

C_S_Edge::~C_S_Edge() {

}

void C_S_Edge::SaveEmbeddedInfo(int _id, double _demand) {
	Embedded_Info embedded_info = {_id, _demand};
	m_embedded_info_vector.push_back(embedded_info);
}

void C_S_Edge::SaveProvisioningInfo(double _bw) {
	provisioned_weight += _bw;
	weight += _bw;
}

void C_S_Edge::UpdateEdgeInfo() {
	int tmp_n_accepted_requests = 0;	
	double tmp_filled_weight = 0;
	
	for (embedded_info_iter iter = m_embedded_info_vector.begin(); iter != m_embedded_info_vector.end(); iter++) {
		tmp_n_accepted_requests++;	
		tmp_filled_weight += (*iter).demand;
	}
	n_accepted_requests = tmp_n_accepted_requests;	
	filled_weight = tmp_filled_weight;
}

void C_S_Edge::PrintEdgeInfo(ostream& out_stream) {
	out_stream << "C_S_Edge[" << edge_ID << "]" << endl;	
	out_stream << "n_accepted_requests\t" << n_accepted_requests << "\tfilled_weight\t" << filled_weight << endl << endl;
}

double C_S_Edge::GetActualQoS() {

	double capacity = weight;
	double qos = filled_weight;

	cout << "c_edgeID\t" << edge_ID << "\tcapacity\t" << capacity << "\tfilled_weight\t" << filled_weight << endl;

	return filled_weight;
}

/* C_Phy_Topology Class */
C_Phy_Topo::C_Phy_Topo(int snodenum, int sedgenum, double capa) {
	int tmp_node_number = 0;	
	int tmp_edge_number = 0;

	total_nodes = snodenum;
	total_edges = sedgenum;
	
	/* create C_S_Edge - C_S_Node map */
	edge_node_map = new int*[sedgenum];
	for (int i=0; i<sedgenum; i++) {
		edge_node_map[i] = new int[2];	// 0: node1, 1: node2
	}
	
	nodes = new C_S_Node*[snodenum];
	edges = new C_S_Edge*[sedgenum];

	/* read topology */
//	ifstream inFile("topo_expr1.txt");
	ifstream inFile("B4_SN_topology.txt");
//	ifstream inFile("B4_SN_topology_test.txt");
//	ifstream inFile("complete.txt");

	/* parsing */
	while (!inFile.eof()) {
		char* token;
		char* current;
		char* buf = c_inputString;

		int adjacent_node = 0;

		inFile.getline(c_inputString, 100);
		
		//SG: have to be modified (parsed with string)
		//if (tmp_node_number == 4) {
		//	break;	
		//}
		
		/* create current C_S_Node */
		nodes[tmp_node_number] = new C_S_Node(tmp_node_number);		

		while ((token = strtok_r(buf, DELIM, &current)) != NULL) {
			buf = NULL;
	
			if (atoi(token) != 0) {	// this node has an edge
				nodes[tmp_node_number]->edge_number++;

				if (nodes[tmp_node_number]->node_ID > adjacent_node) {
					for (int i=0; i<sedgenum; i++) {
						if(edge_node_map[i][1] == tmp_node_number) {
							break;		
						}
					}
				}
				else {
					/* create C_S_Edge */		
					edge_node_map[tmp_edge_number][0] = tmp_node_number;
					edge_node_map[tmp_edge_number][1] = adjacent_node;
					edges[tmp_edge_number] = new C_S_Edge(tmp_edge_number
							, nodes[tmp_node_number]->node_ID, adjacent_node, capa);
					tmp_edge_number++;	
				}
			}
			adjacent_node++;
		}
		tmp_node_number++;	
	}
	inFile.close();

	//Trace
	/*for (int i=0; i<snodenum; i++) {
		cout << "nodeID: " << nodes[i]->node_ID << "\tedge_number: " << nodes[i]->edge_number << endl;
	}*/

	//Trace
	/*for (int i=0; i<sedgenum; i++) {
		cout << "edgeID: " << edges[i]->edge_ID << "\tnode1: " << edges[i]->node1 << "\tnode2: " << edges[i]->node2 << endl;
	}*/

	//Trace
	/*for (int i=0; i<sedgenum; i++) {
		for (int j=0; j<2; j++) {
			cout << "edge_node_map[" << i << "][" << j << "]: " << edge_node_map[i][j] << endl;
		}
	}*/
}

C_Phy_Topo::~C_Phy_Topo() {

}

void C_Phy_Topo::UpdateAllEdgeInfo() {
	for (int i=0; i<total_edges; i++) {
		edges[i]->UpdateEdgeInfo();
	}
}
void C_Phy_Topo::PrintAllNodeInfo() {
	
}

void C_Phy_Topo::PrintAllEdgeInfo(ostream& out_stream) {
	for (int i=0; i<total_edges; i++) {
		edges[i]->PrintEdgeInfo(out_stream);	
	}
}
