#include <fstream>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <algorithm>	// for std::minimum
#include <math.h>

#include "Phy_topology.h"
#include "Normsinv.h"
#include "Normcdf.h"

#define MAX_SIZE 100
#define DELIM "\t\n"

char inputString[MAX_SIZE];

using namespace std;

/* S_Node Class */
S_Node::S_Node(int node_number) {
	node_ID = node_number;
	edge_number = 0;
}

S_Node::~S_Node() {

}

void S_Node::PrintNodeInfo() {

}

/* S_Edge Class */
S_Edge::S_Edge() {

}

S_Edge::S_Edge(int edge_number, int current_node, int adjacent_node, int capacity) {
	edge_ID = edge_number;
	node1 = current_node;
	node2 = adjacent_node;
	weight = capacity;

	provisioned_weight = 0.0;

	n_accepted_requests = 0;
	filled_weight = 0;
	total_var = 0.0;
	min_epsilon = 1.0;
}

S_Edge::~S_Edge() {

}

void S_Edge::SaveEmbeddedInfo(int _id, double _mean, double _var, double _eps) {
	Embedded_Info embedded_info = {_id, _mean, _var, _eps};
	m_embedded_info_vector.push_back(embedded_info);
}

void S_Edge::SaveProvisioningInfo(double _bw) {
	provisioned_weight += _bw;
	weight += _bw;
}

void S_Edge::UpdateEdgeInfo() {
	int tmp_n_accepted_requests = 0;	
	int tmp_filled_weight = 0;
	double tmp_total_var = 0.0;
	double tmp_min_epsilon = 1.0;
	double tmp_provisioned_bw = 0.0;

	for (embedded_info_iter iter = m_embedded_info_vector.begin(); iter != m_embedded_info_vector.end(); iter++) {
		tmp_n_accepted_requests++;	
		tmp_filled_weight += (*iter).mean;
		tmp_total_var += (*iter).var;
		tmp_min_epsilon = min(tmp_min_epsilon, (*iter).epsilon);	
	}
	n_accepted_requests = tmp_n_accepted_requests;	
	filled_weight = tmp_filled_weight;
	total_var = tmp_total_var;
	min_epsilon = tmp_min_epsilon;
}

void S_Edge::PrintEdgeInfo(ostream& out_stream) {
	out_stream << "S_Edge[" << edge_ID << "]" << endl;	
	out_stream << "n_accepted_requests\t" << n_accepted_requests << "\tfilled_weight\t" << filled_weight << "\ttotal_var\t" << total_var
		<< "\tmin_epsilon\t" << min_epsilon << endl;
	out_stream << "accepted_requests\t";

	for (embedded_info_iter iter = m_embedded_info_vector.begin(); iter != m_embedded_info_vector.end(); iter++) {
		out_stream << (*iter).embedded_request_ID << "\t";
	}
	out_stream << endl << endl;
}

double S_Edge::GetActualQoS() {
	double qos;

	double capacity = weight;
	double mean = filled_weight;
	double var = total_var;

	cout << "edgeID\t" << edge_ID << "\tcapacity\t" << capacity << "\tmean\t" << mean << "\tvar\t" << var << "\tstddev\t" << sqrt(var)
		<< "\tvalue\t" << (double(capacity) - mean)/sqrt(var) << "\tqos\t" << phi((double(capacity) - mean)/sqrt(var)) << endl;

	qos = phi((double(capacity) - mean)/sqrt(var));
	return qos;
}

/* Phy_Topology Class */
/*
Phy_Topo::Phy_Topo(int snodenum, int sedgenum, int capa) {
	int tmp_node_number = 0;	
	int tmp_edge_number = 0;

	total_nodes = snodenum;
	total_edges = sedgenum;
	
	// create S_Edge - S_Node map
	edge_node_map = new int*[sedgenum];
	for (int i=0; i<sedgenum; i++) {
		edge_node_map[i] = new int[2];	// 0: node1, 1: node2
	}
	
	nodes = new S_Node*[snodenum];
	edges = new S_Edge*[sedgenum];

	// read topology
	ifstream inFile("B4_SN_topology.txt");

	// parsing	
	while (!inFile.eof()) {
		char* token;
		char* current;
		char* buf = inputString;

		int adjacent_node = 0;

		inFile.getline(inputString, 100);
		
		//SG: have to be modified (parsed with string)
		if (tmp_node_number == 12) {
			break;	
		}
		
		// create current S_Node
		nodes[tmp_node_number] = new S_Node(tmp_node_number);		

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
					// create S_Edge 		
					edge_node_map[tmp_edge_number][0] = tmp_node_number;
					edge_node_map[tmp_edge_number][1] = adjacent_node;
					edges[tmp_edge_number] = new S_Edge(tmp_edge_number
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
	//for (int i=0; i<snodenum; i++) {
	//	cout << "nodeID: " << nodes[i]->node_ID << "\tedge_number: " << nodes[i]->edge_number << endl;
	//}

	//Trace
	//for (int i=0; i<sedgenum; i++) {
//		cout << "edgeID: " << edges[i]->edge_ID << "\tnode1: " << edges[i]->node1 << "\tnode2: " << edges[i]->node2 << endl;
//	}
//	
	//Trace
	//for (int i=0; i<sedgenum; i++) {
	//	for (int j=0; j<2; j++) {
	//		cout << "edge_node_map[" << i << "][" << j << "]: " << edge_node_map[i][j] << endl;
	//	}
	//}
}
*/

Phy_Topo::Phy_Topo(int snodenum, int sedgenum, int capa) {
	int tmp_node_number = 0;	
	int tmp_edge_number = 0;

	total_nodes = snodenum;
	total_edges = sedgenum;

	/* create S_Edge - S_Node map */
	edge_node_map = new int*[sedgenum];
	for (int i=0; i<sedgenum; i++) {
		edge_node_map[i] = new int[2];	// 0: node1, 1: node2
	}
	
	nodes = new S_Node*[snodenum];
	edges = new S_Edge*[sedgenum];

	/* read topology */
	ifstream inFile("topo_expr1.txt");
//	ifstream inFile("B4_SN_topology.txt");
//	ifstream inFile("B4_SN_topology_test.txt");
//	ifstream inFile("complete.txt");

	/* parsing */
	while (!inFile.eof()) {
		char* token;
		char* current;
		char* buf = inputString;

		int adjacent_node = 0;

		inFile.getline(inputString, 100);
		
		//SG: have to be modified (parsed with string)
		cout << tmp_node_number << endl;
		if (tmp_node_number == 4) {
			break;	
		}
		
		/* create current S_Node */
		nodes[tmp_node_number] = new S_Node(tmp_node_number);		

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
					/* create S_Edge */		
					edge_node_map[tmp_edge_number][0] = tmp_node_number;
					edge_node_map[tmp_edge_number][1] = adjacent_node;
					edges[tmp_edge_number] = new S_Edge(tmp_edge_number
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
	}

	//Trace
	for (int i=0; i<sedgenum; i++) {
		cout << "edgeID: " << edges[i]->edge_ID << "\tnode1: " << edges[i]->node1 << "\tnode2: " << edges[i]->node2 << endl;
	}
	*/
	//Trace
	/*for (int i=0; i<sedgenum; i++) {
		for (int j=0; j<2; j++) {
			cout << "edge_node_map[" << i << "][" << j << "]: " << edge_node_map[i][j] << endl;
		}
	}*/
}

Phy_Topo::~Phy_Topo() {

}

void Phy_Topo::UpdateAllEdgeInfo() {
	for (int i=0; i<total_edges; i++) {
		cout << "i: " << i << "\ttotal: " << total_edges << endl;	
		edges[i]->UpdateEdgeInfo();
	}
}

void Phy_Topo::PrintAllNodeInfo() {
	
}

void Phy_Topo::PrintAllEdgeInfo(ostream& out_stream) {
	for (int i=0; i<total_edges; i++) {
		edges[i]->PrintEdgeInfo(out_stream);	
	}
}
