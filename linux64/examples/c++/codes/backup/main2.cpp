#include "gurobi_c++.h"

#include <iostream>
#include <stdlib.h>		// for rand
#include <time.h>		// for srand
#include <string.h>		// for strtok_r
#include <sstream>
#include <functional>	// for sorting

/* for k-shortest paths */
#include <limits>
#include <set>
#include <map>
#include <queue>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>	// for std::minimum

#include "Normsinv.h"
#include "Phy_topology.h"
#include "Request.h"
#include "C_Phy_topology.h"
#include "C_Request.h"

/* for k-shortest paths */
#include "GraphElements.h"
#include "Graph.h"
#include "DijkstraShortestPathAlg.h"
#include "YenTopKShortestPathsAlg.h"

#define SNODENUM 12		// number of data centers: 11
#define SEDGENUM 19		// number of edges: 18
#define CAPACITY 160	// capacity: 160Gbps
#define MAX_PATH_SIZE 10	// all paths could not consist of more than 10 edges
#define scaling_constant 0.001	// link_constraints should be changed with linear form 
#define MAX_FLOW_SIZE 3	// the maximum number of flows per each request

using namespace std;

/* initialization of static variable in Request Class */
int Request::count = 0;
int C_Request::c_count = 0;

/* function declaration */
void init_request(Request*, C_Request*, int, bool, double);
void find_k_shortest_paths(Request, int, ostream&, ostream&);
void find_k_shortest_paths_for_new_requests(Request, int, int, ostream&);
void find_k_shortest_paths_for_c_new_requests(C_Request, int, int, ostream&);
void ChangePathform(Request*, vector <vector <int> >&, Phy_Topo*);	
void C_ChangePathform(C_Request*, vector <vector <int> >&, C_Phy_Topo*);	
vector <int> ChangeNewRequestPathsform(Request*, vector <int>&, Phy_Topo*);
vector <int> C_ChangeNewRequestPathsform(C_Request*, vector <int>&, C_Phy_Topo*);
void testDijkstraGraph();
void testYenAlg();
string itos(int i) {stringstream s; s << i; return s.str(); }

enum Alg {
	Relax_Var,
	Strong_Var
};

bool relax_debug = false;

//int main() {
int main(int argc, char* argv[]) {
	//srand(time(NULL));
	srand(atoi(argv[1]));

	int n_embedded_request = 0;	// the number of embedded requests;
	int n_request = 40;	// the number of new requests
	int top_k;	// consider only top_k shortest paths

	double req_qos = atof(argv[2]);

	double approximation_slope = atof(argv[3]);
	double approximation_constant = atof(argv[4]);
	double expansion_point = atof(argv[5]);

	char node_arr_for_path[100];

	bool trace_on = true;

	enum Alg alg = Relax_Var;
/*
	int alg_num = Strong_Var;
	alg_num = atof(argv[6]);

	if (alg_num == 1)
		alg = Strong_Var;
	else
		alg = Relax_Var;
*/
	if (alg == Relax_Var) {
		top_k = 10;
	}
	else {
		top_k = 10;
	}

	/* initialization of substrate network */
	Phy_Topo *Topo;
	Topo = new Phy_Topo(SNODENUM, SEDGENUM, CAPACITY);

	/* initialization of new substrate network for the comparison group */
	C_Phy_Topo *C_Topo;
	C_Topo = new C_Phy_Topo(SNODENUM, SEDGENUM, CAPACITY);

/* ======================================== embedded requests (background) ======================================== */
	
	/* initialization of embedded requests */
	Request embedded_request[n_embedded_request];
	C_Request c_embedded_request[n_embedded_request];	
	init_request(embedded_request, c_embedded_request, n_embedded_request, true, req_qos);

	/* find the shortest path for "embedded_request" */
	ofstream out_embedded_File("environment/embedded_requests_path.txt");
	ofstream out_c_embedded_File("environment/c_embedded_requests_path.txt");
	for (int i=0; i<n_embedded_request; i++) {
		out_embedded_File << "embedded_request[" << i << "]\tsrc\t" << embedded_request[i].src_node << "\tdst\t"
			<< embedded_request[i].dest_node << "\tmean\t" << embedded_request[i].mean << "\tvar\t" << embedded_request[i].var
			<< "\tepsilon\t" << embedded_request[i].epsilon << endl;
		out_c_embedded_File << "c_embedded_request[" << i << "]\tsrc\t" << c_embedded_request[i].src_node << "\tdst\t"
			<< c_embedded_request[i].dest_node << "\tdemand\t" << c_embedded_request[i].demand << endl;
		find_k_shortest_paths(embedded_request[i], 1, out_embedded_File, out_c_embedded_File);
	}
	out_embedded_File << "the end" << endl;	
	out_c_embedded_File << "the end" << endl;	
	out_embedded_File.close();
	out_c_embedded_File.close();

	ifstream in_embedded_File("environment/embedded_requests_path.txt");
	int embedded_user_count = 0;
	vector <vector <int> > embedded_user_path_vector;	// 2 dimensions vector (user - path) to save the shortest path of each requests

	while (!in_embedded_File.eof()) {
		char* token;
		char* current;
		char* buf = node_arr_for_path;
		
		in_embedded_File.getline(node_arr_for_path, 100); 

		if (*node_arr_for_path == 'e') {
			embedded_user_path_vector.push_back(vector <int>());
			embedded_user_count++;	
		}
		else if (*node_arr_for_path == 'c') {
			// do nothing	
		}
		else if (*node_arr_for_path == 't') {
			break;	// text is finished
		}
		else {
			while ((token = strtok_r(buf, " ", &current)) != NULL) {
				buf = NULL;
				embedded_user_path_vector[embedded_user_count-1].push_back(atoi(token));	// save the shortest path of each embedded requests
			}
		}
	}

	/* change form of a path from node to edge & save the information of embedded requests on each edges */
	ChangePathform(embedded_request, embedded_user_path_vector, Topo);	
	C_ChangePathform(c_embedded_request, embedded_user_path_vector, C_Topo);
	
	/* update topology */
	Topo->UpdateAllEdgeInfo();
	C_Topo->UpdateAllEdgeInfo();

	// Trace
	ofstream out_edge_File("environment/edge.txt");
	ofstream out_c_edge_File("environment/c_edge.txt");
	Topo->PrintAllEdgeInfo(out_edge_File);
	C_Topo->PrintAllEdgeInfo(out_c_edge_File);
	out_edge_File.close();
	out_c_edge_File.close();

/* ======================================== new requests ======================================== */
	
	/* initialization of new requests */
	Request request[n_request];	
	C_Request c_request[n_request];	
	init_request(request, c_request, n_request, false, req_qos);

/*
	// Trace
	ofstream before_sort_File("environment/before_sort_arr.txt");
	ofstream after_sort_File("environment/after_sort_arr.txt");	
	ofstream c_before_sort_File("environment/c_before_sort_arr.txt");
	ofstream c_after_sort_File("environment/c_after_sort_arr.txt");	
	
	for (int i=0; i<n_request; i++) {
		before_sort_File << request[i].request_ID << "\t" << request[i].total_mean << endl;		
	}
	for (int i=0; i<n_request; i++) {
		c_before_sort_File << c_request[i].request_ID << "\t" << c_request[i].total_demand << endl;		
	}
*/

	// Sort requests as descending order
	sort(request, request + n_request);
	sort(c_request, c_request + n_request);

	/*
	// Trace
	for (int i=0; i<n_request; i++) {
		after_sort_File << request[i].request_ID << "\t" << request[i].total_mean << endl;		
	}
	for (int i=0; i<n_request; i++) {
		c_after_sort_File << c_request[i].request_ID << "\t" << c_request[i].total_demand << endl;		
	}
	before_sort_File.close();	
	c_before_sort_File.close();	
	after_sort_File.close();	
	c_after_sort_File.close();	

	/* link_indicator : used for linear programming */
	int link_indicator[n_request][MAX_FLOW_SIZE][top_k][SEDGENUM];
	int c_link_indicator[n_request][MAX_FLOW_SIZE][top_k][SEDGENUM];

	for (int i=0; i<n_request; i++) {
		for (int j=0; j<MAX_FLOW_SIZE; j++) {
			for (int k=0; k<top_k; k++) {
				for (int l=0; l<SEDGENUM; l++) {
					link_indicator[i][j][k][l] = 0;	
					c_link_indicator[i][j][k][l] = 0;	
				}
			}
		}
	}
	
	/* find k-shortest paths for new and c_new requests */
	vector <int> new_req_path_vec[n_request][MAX_FLOW_SIZE][top_k];
	ofstream out_new_File("environment/new_requests_path.txt");

	for (int i=0; i<n_request; i++) {
		out_new_File << "new_request[" << i << "]" << endl;	
		
		int flow_number = request[i].flow_count;
		
		for (int j=0; j<flow_number; j++) {
			out_new_File << "[" << i << "][" << j << "]\tsrc\t" << request[i].flows[j]->src2 << "\tdst\t" << request[i].flows[j]->dst2 << "\tmean\t"
			<< request[i].flows[j]->mean2 << "\tvar\t" << request[i].flows[j]->var2 << "\tepsilon\t" << request[i].flows[j]->epsilon2 << endl;
			find_k_shortest_paths_for_new_requests(request[i], j, top_k, out_new_File);
		}
	}
	out_new_File << "the end" << endl;	
	out_new_File.close();	

	vector <int> c_new_req_path_vec[n_request][MAX_FLOW_SIZE][top_k];
	ofstream out_c_new_File("environment/c_new_requests_path.txt");	
	for (int i=0; i<n_request; i++) {
		out_c_new_File << "new_request[" << i << "]" << endl;
		
		int flow_number = c_request[i].c_flow_count;
		
		for (int j=0; j<flow_number; j++) {
			out_c_new_File << "[" << i << "][" << j << "]\tsrc\t" << c_request[i].c_flows[j]->src2 << "\tdst\t" << c_request[i].c_flows[j]->dst2 << "\tdemand\t"
			<< c_request[i].c_flows[j]->demand2 << endl;
			find_k_shortest_paths_for_c_new_requests(c_request[i], j, top_k, out_c_new_File);
		}
	}
	out_c_new_File << "the end" << endl;	
	out_c_new_File.close();	

	/* store k-shortest paths of each requests with edge_ID form */
	ifstream in_new_File("environment/new_requests_path.txt");
	int user_count = 0;	
	while (!in_new_File.eof()) {
		int path_count;	
		int flow_count;

		char* token;
		char* current;
		char* buf = node_arr_for_path;
	
		in_new_File.getline(node_arr_for_path, 100); 

		if (*node_arr_for_path == 'n') {	// new request
			path_count = 0;	
			flow_count = 0;	
			user_count++;
		}
		else if (*node_arr_for_path == '[') {	// flow
			flow_count++;
			path_count = 0;	
		}
		else if (*node_arr_for_path == 'c') {	//	cost, length 
			path_count++;	
		}
		else if (*node_arr_for_path == 't') {	
			break;	// the text is finished
		}
		else {
			vector <int> test_vector;	
			
			while ((token = strtok_r(buf, " ", &current)) != NULL) {
				buf = NULL;

				test_vector.push_back(atoi(token));
			}
			new_req_path_vec[user_count-1][flow_count-1][path_count-1] = ChangeNewRequestPathsform(request, test_vector, Topo);	// change path form 	
		}
	}
	
	ifstream c_in_new_File("environment/c_new_requests_path.txt");	
	user_count = 0;
	while (!c_in_new_File.eof()) {
		int path_count;	
		int flow_count;

		char* token;
		char* current;
		char* buf = node_arr_for_path;
	
		c_in_new_File.getline(node_arr_for_path, 100); 

		if (*node_arr_for_path == 'n') {	// new request
			path_count = 0;	
			flow_count = 0;	
			user_count++;
		}
		else if (*node_arr_for_path == '[') {	// flow
			flow_count++;
			path_count = 0;	
		}
		else if (*node_arr_for_path == 'c') {	//	cost, length 
			path_count++;	
		}
		else if (*node_arr_for_path == 't') {	
			break;	// the text is finished
		}
		else {
			vector <int> test_vector;	
			
			while ((token = strtok_r(buf, " ", &current)) != NULL) {
				buf = NULL;

				test_vector.push_back(atoi(token));
			}
			c_new_req_path_vec[user_count-1][flow_count-1][path_count-1] = C_ChangeNewRequestPathsform(c_request, test_vector, C_Topo);	// change path form 	
		}
	}

	/* update link_indicator matrix */
	vector <int>::iterator path_iter;
	for (int i=0; i<n_request; i++) {
		for (int j=0; j<MAX_FLOW_SIZE; j++) {
			for (int k=0; k<top_k; k++) {
				for (path_iter = new_req_path_vec[i][j][k].begin(); path_iter != new_req_path_vec[i][j][k].end(); path_iter++) {
					link_indicator[i][j][k][*(path_iter)] = 1; 
				}
			}
		}
	}
	for (int i=0; i<n_request; i++) {
		for (int j=0; j<MAX_FLOW_SIZE; j++) {
			for (int k=0; k<top_k; k++) {
				for (path_iter = c_new_req_path_vec[i][j][k].begin(); path_iter != c_new_req_path_vec[i][j][k].end(); path_iter++) {
					c_link_indicator[i][j][k][*(path_iter)] = 1; 
				}
			}
		}
	}
	
	/*	
	// Trace
	for (int i=0; i<n_request; i++) {
		for (int j=0; j<MAX_FLOW_SIZE; j++) {
			cout << "request[" << i << "][" << j << "]" << endl;	
			for (int k=0; k<top_k; k++) {
				for (path_iter = new_req_path_vec[i][j][k].begin(); path_iter != new_req_path_vec[i][j][k].end(); path_iter++) {
						cout << *(path_iter) << "-> "; 
				}
			cout << endl;	
			}
		}
	}
	for (int i=0; i<n_request; i++) {
		for (int j=0; j<MAX_FLOW_SIZE; j++) {
			cout << "c_request[" << i << "][" << j << "]" << endl;	
			for (int k=0; k<top_k; k++) {
				for (path_iter = c_new_req_path_vec[i][j][k].begin(); path_iter != c_new_req_path_vec[i][j][k].end(); path_iter++) {
						cout << *(path_iter) << "-> "; 
				}
			cout << endl;	
			}
		}
	}
	*/	

/* ======================================== LP (our model, minimize flow, greedy, gurobi) ======================================== */

	for (int i=0; i<n_request; i++) {
		int flow_number;	

		// have to be initialized for each trial (strong_var)	
		int var_added_indicator_for_strong_var[MAX_FLOW_SIZE][SEDGENUM];								
		for (int j=0; j<MAX_FLOW_SIZE; j++) {
			for (int l=0; l<SEDGENUM; l++) {
				var_added_indicator_for_strong_var[j][l] = 0;	
			}
		}

		try {
			// create the environment and the model (default)
			GRBEnv env = GRBEnv();
			GRBModel model = GRBModel(env);	
	
			// create variables : path_allocator (1 if a request is embedded on the path, 0 otherwise)
			GRBVar path_allocator[MAX_FLOW_SIZE][top_k];		
			for (int j=0; j<MAX_FLOW_SIZE; j++) {
				for (int k=0; k<top_k; k++) {
					string s = "R" + itos(i) + "F" + itos(j) + "P" + itos(k);
					path_allocator[j][k] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, s);
				}
			}

			// intergrate variables into model
			model.update();	

			// set objective: minimize total flow
			GRBLinExpr obj = 0;	
			flow_number = request[i].flow_count;	
			for (int j=0; j<MAX_FLOW_SIZE; j++) {
				for (int k=0; k<top_k; k++) {
					for (int l=0; l<SEDGENUM; l++) {
						if (j<flow_number)	
							obj += request[i].flows[j]->mean2 * path_allocator[j][k] * link_indicator[i][j][k][l];
						else {
							// do nothing
						}
					}
				}
			}
			model.setObjective(obj, GRB_MINIMIZE);	

			// add constraint: each flow must use only 1 path
			GRBLinExpr path_constraint_expr;
			for (int j=0; j<MAX_FLOW_SIZE; j++) {
				path_constraint_expr = 0;
				string s = "PATH_CONSTRAINT_R" + itos(i) + "_F" + itos(j);	
				for (int k=0; k<top_k; k++) {
					path_constraint_expr += path_allocator[j][k];
				}
				if (j<flow_number)	
					model.addConstr(path_constraint_expr == 1.0, s);	
				else
					model.addConstr(path_constraint_expr == 0.0, s);	
			}	

			// add constraint: link constraints
			// approximation
			switch (alg) {
				case Relax_Var:
					for (int l=0; l<SEDGENUM; l++) {
						GRBLinExpr link_constraint_expr;
						GRBLinExpr link_constraint_expr_mean = Topo->edges[l]->filled_weight;
				
						double constraint_expr_var = Topo->edges[l]->total_var;

						double link_weight = Topo->edges[l]->weight;
						double link_min_eps = Topo->edges[l]->min_epsilon;

						for (int j=0; j<flow_number; j++) {
							for (int k=0; k<top_k; k++) {
								link_constraint_expr_mean += request[i].flows[j]->mean2 * path_allocator[j][k] * link_indicator[i][j][k][l];
						
								if (link_indicator[i][j][k][l] == 1) {	// variance should be reflected on the link l
									for (path_iter = new_req_path_vec[i][j][k].begin(); path_iter != new_req_path_vec[i][j][k].end(); path_iter++) {
										if (l == *(path_iter)) {
											constraint_expr_var += request[i].flows[j]->var2 * (double(1.0)/double(top_k));
											link_min_eps = min(Topo->edges[l]->min_epsilon, request[i].flows[j]->epsilon2);
										}
										else {
											// do nothing (already reflected)
										}
									}	
								}
							}	// path (k)
						}	// flow (j)
						
						// approximation
						//link_constraint_expr = (0.120406) - (0.189989) * (((link_weight - link_constraint_expr_mean) / sqrt(constraint_expr_var)) - 1.281);
						link_constraint_expr = (approximation_constant) - (approximation_slope) * (((link_weight - link_constraint_expr_mean) / sqrt(constraint_expr_var)) - expansion_point);
						string s = "LINK_CONSTRAINT_L" + itos(l);
						model.addConstr(link_constraint_expr <= link_min_eps, s);		
					}
					break;
				case Strong_Var:
					for (int l=0; l<SEDGENUM; l++) {
						GRBLinExpr link_constraint_expr;
						GRBLinExpr link_constraint_expr_mean = Topo->edges[l]->filled_weight;

						double constraint_expr_var = Topo->edges[l]->total_var;

						double link_weight = Topo->edges[l]->weight;
						double link_min_eps = Topo->edges[l]->min_epsilon;

						for (int j=0; j<flow_number; j++) {
							for (int k=0; k<top_k; k++) {
								link_constraint_expr_mean += request[i].flows[j]->mean2 * path_allocator[j][k] * link_indicator[i][j][k][l];
						
								if (link_indicator[i][j][k][l] == 1) {	// variance should be reflected on the link l
									for (path_iter = new_req_path_vec[i][j][k].begin(); path_iter != new_req_path_vec[i][j][k].end(); path_iter++) {
										if (l == *(path_iter)) {
											if (var_added_indicator_for_strong_var[j][l] == 0) {
												constraint_expr_var += request[i].flows[j]->var2;
												var_added_indicator_for_strong_var[j][l] = 1;
												link_min_eps = min(Topo->edges[l]->min_epsilon, request[i].flows[j]->epsilon2);
												//link_min_eps = 0.1;	
											}
											else {
												// do nothing (already reflected)
												//link_min_eps = 0.1;	
											}
										}	
									}
								}
							}	// path (k)
						}	// flow (j)
					
						// approximation
						//link_constraint_expr = (0.120406) - (0.189989) * (((link_weight - link_constraint_expr_mean) / sqrt(constraint_expr_var)) - 1.281);
						link_constraint_expr = (approximation_constant) - (approximation_slope) * (((link_weight - link_constraint_expr_mean) / sqrt(constraint_expr_var)) - expansion_point);
						string s = "LINK_CONSTRAINT_L" + itos(l);
						model.addConstr(link_constraint_expr <= link_min_eps, s);		
					}	// edge (l)
					break;
				default:
					break;
			}

			// optimize model
			model.optimize();

			// decide each requests can be accepted or not
			int path_allocator_sum;	
			flow_number = request[i].flow_count;	
			path_allocator_sum = 0;
			for (int j=0; j<flow_number; j++) {
				for (int k=0; k<top_k; k++) {
					path_allocator_sum += path_allocator[j][k].get(GRB_DoubleAttr_X);
				}
			}	
			if (path_allocator_sum == flow_number) {	// this request is accepted
				request[i].accepted = true;

				for (int j=0; j<flow_number; j++) {
					for (int k=0; k<top_k; k++) {	
						if (path_allocator[j][k].get(GRB_DoubleAttr_X) == 1) {	// the flow j should be embedded on path k
							for (path_iter = new_req_path_vec[i][j][k].begin(); path_iter != new_req_path_vec[i][j][k].end(); path_iter++) {
								int ID = i;

								double m = request[i].flows[j]->mean2;
								double v = request[i].flows[j]->var2;
								double e = request[i].flows[j]->epsilon2;

								cout << "ID: " << ID << ", FLOW: " << j << ", Path: " << k << ", mean: " << m << ", var: " << v << ", eps: " << e << endl;
								Topo->edges[*path_iter]->SaveEmbeddedInfo(ID, m, v, e);
							}
						}
					}
				}
			}

			Topo->UpdateAllEdgeInfo();
			//Topo->PrintAllEdgeInfo(cout);

		}	// optimze
		catch(GRBException e) {
		cout << "Error code = " << e.getErrorCode() << endl;
		cout << e.getMessage() << endl;
		} 
		catch(...) {
		cout << "Exception during optimization" << endl;
		}	
	}	// for each request (i)

	cout << "============================================== OUR MODEL END ===================================================" << endl;
	cout << endl;

/* ======================================== LP (comparison group, greedy) ======================================== */

	for (int i=0; i<n_request; i++) {
		int c_flow_number;	
	
		try {
			// create the environment and the model (default)
			GRBEnv c_env = GRBEnv();
			GRBModel c_model = GRBModel(c_env);	

			// create variables : path_allocator (1 if a request is embedded on the path, 0 otherwise)
			GRBVar c_path_allocator[MAX_FLOW_SIZE][top_k];		
			for (int j=0; j<MAX_FLOW_SIZE; j++) {
				for (int k=0; k<top_k; k++) {
					string s = "C_R" + itos(i) + "F" + itos(j) + "P" + itos(k);
					c_path_allocator[j][k] = c_model.addVar(0.0, 1.0, 0.0, GRB_BINARY, s);
				}
			}

			// intergrate variables into model
			c_model.update();	

			// set objective: minimize total flow
			GRBLinExpr c_obj = 0;	
			c_flow_number = c_request[i].c_flow_count;	
			for (int j=0; j<MAX_FLOW_SIZE; j++) {
				for (int k=0; k<top_k; k++) {
					for (int l=0; l<SEDGENUM; l++) {
						if (j<c_flow_number)	
							c_obj += c_request[i].c_flows[j]->demand2 * c_path_allocator[j][k] * c_link_indicator[i][j][k][l];
						else {
							// do nothing
						}
					}
				}
			}
			c_model.setObjective(c_obj, GRB_MINIMIZE);	
			
			// add constraint: each flow must use only 1 path
			GRBLinExpr c_path_constraint_expr;
			for (int j=0; j<MAX_FLOW_SIZE; j++) {
				c_path_constraint_expr = 0;
				string s = "C_PATH_CONSTRAINT_R" + itos(i) + "_F" + itos(j);	
				for (int k=0; k<top_k; k++) {
					c_path_constraint_expr += c_path_allocator[j][k];
				}
				if (j<c_flow_number)	
					c_model.addConstr(c_path_constraint_expr == 1.0, s);	
				else
					c_model.addConstr(c_path_constraint_expr == 0.0, s);	
			}	

			// add constraint: link constraints 
			GRBLinExpr c_link_constraint_expr;		
			for (int l=0; l<SEDGENUM; l++) {
				c_link_constraint_expr = C_Topo->edges[l]->filled_weight;

				double link_weight = C_Topo->edges[l]->weight;

				for (int j=0; j<c_flow_number; j++) {
					for (int k=0; k<top_k; k++) {
					//	cout << "k: " << k << ", i: " << i << ", j: " << j << ", value: " << c_request[i].demand * c_link_indicator[i][j][k] << endl;	
						c_link_constraint_expr += c_request[i].c_flows[j]->demand2 * c_path_allocator[j][k] * c_link_indicator[i][j][k][l];	
					}
				}
				string s = "C_LINK_CONSTRAINT_L" + itos(l);
				c_model.addConstr(c_link_constraint_expr <= link_weight, s);		
			}

			// optimize model
			c_model.optimize();

			// decide each requests can be accepted or not
			int c_path_allocator_sum = 0;	
			for (int j=0; j<c_flow_number; j++) {
				for (int k=0; k<top_k; k++) {
					c_path_allocator_sum += c_path_allocator[j][k].get(GRB_DoubleAttr_X);
				}
			}	
			if (c_path_allocator_sum == c_flow_number) {	// this request is accepted
				c_request[i].accepted = true;

				for (int j=0; j<c_flow_number; j++) {
					for (int k=0; k<top_k; k++) {	
						if (c_path_allocator[j][k].get(GRB_DoubleAttr_X) == 1) {	// the flow j should be embedded on path k 
							for (path_iter = c_new_req_path_vec[i][j][k].begin(); path_iter != c_new_req_path_vec[i][j][k].end(); path_iter++) {
								int ID = i;
								double demand = c_request[i].c_flows[j]->demand2;

								cout << "ID: " << ID << ", FLOW: " << j << ", Path: " << k << ", DEMAND: " << demand << ", EDGE: " << C_Topo->edges[*path_iter]->edge_ID << endl;
								C_Topo->edges[*path_iter]->SaveEmbeddedInfo(ID, demand);
							}
						}
					}
				}
			}

			C_Topo->UpdateAllEdgeInfo();
			//C_Topo->PrintAllEdgeInfo(cout);

		}	// try (optimize)

		catch(GRBException e) {
		cout << "Error code = " << e.getErrorCode() << endl;
		cout << e.getMessage() << endl;
		} 
		catch(...) {
		cout << "Exception during optimization" << endl;
		}	
	}	// for each request (i)

/* ======================================== Tracing ======================================== */

	// the number of accepted requests
	//ofstream acc_req_result_File("data/accepted_requests_number.txt");
	//ofstream c_acc_req_result_File("data/c_accepted_requests_number.txt");

	//C_Topo->PrintAllEdgeInfo(cout);
	//Topo->PrintAllEdgeInfo(cout);
	
	ofstream acc_req_result_File;
	ofstream c_acc_req_result_File;
	acc_req_result_File.open("data/accepted_requests", ios_base::out | ios_base::app);
	c_acc_req_result_File.open("data/c_accepted_requests", ios_base::out | ios_base::app);

	if (trace_on == true) {
		cout << endl;	
		cout << "/* ======================================== # of new accepted requests ======================================== */" << endl << endl;
		cout << "********** our model **********" << endl << endl;
		int accepted_count = 0;	
		for (int i=0; i<n_request; i++) {
			cout << "request " << i << "\taccepted\t" << request[i].accepted << endl;	
			
			if (request[i].accepted == true) {
				accepted_count++;
			}
		}
		cout << "# of new accepted requests\t" << accepted_count << "\ttotal_new_requests\t" << n_request << "\treq_qos\t" << req_qos << endl;
		
		// actual qos (feasibility) check
		double qos = 0;	
		double min_qos = 1.0;	
		double tmp_qos = 0;	
		bool violate_constraints = false;	
		for (int i=0; i<SEDGENUM; i++) {
			tmp_qos = Topo->edges[i]->GetActualQoS();
			min_qos = min(min_qos, tmp_qos);

			if (tmp_qos < (1-req_qos)) {
				violate_constraints = true;
			}
			else {
				//do nothing
			}
			qos += tmp_qos;
		}
		cout << "sum_qos\t" << qos << "\taverage_qos\t" << qos/(SEDGENUM) << endl;

		acc_req_result_File << accepted_count << "\t" << n_request << "\t" << min_qos << "\t" << violate_constraints << "\t" << req_qos << endl;
		cout << accepted_count << "\t" << n_request << "\t" << min_qos << "\t" << violate_constraints << "\t" << req_qos << endl;
	//	acc_req_result_File << accepted_count << "\t" << n_request << endl;
		acc_req_result_File.close();	
		cout << endl << endl;

		cout << "********** comparison group **********" << endl << endl;
		accepted_count = 0;	
		for (int i=0; i<n_request; i++) {
			if (c_request[i].accepted == true) {
				accepted_count++;
			}
		}
		cout << "# of new accepted c_requests\t" << accepted_count << "\ttotal_new_requests\t" << n_request << endl;
		c_acc_req_result_File << accepted_count << "\t" << n_request << "\t" << req_qos << endl;
	//	c_acc_req_result_File << accepted_count << "\t" << n_request << endl;
		c_acc_req_result_File.close();	

		cout << endl;
		cout << endl;
		
		// utilization (used bandwidth)
		//cout << "/* ======================================== utilization ======================================== */" << endl << endl;
		/*cout << "********** our model **********" << endl << endl;
		double used_BW = 0;
		int total_BW = SEDGENUM * CAPACITY;
		for (int i=0; i<SEDGENUM; i++) {
			if (Topo->edges[i]->n_accepted_requests != 0)	
				used_BW += (Topo->edges[i]->filled_weight + sqrt(Topo->edges[i]->total_var) * normsinv(double(1.0 - Topo->edges[i]->min_epsilon)));
		}
		cout << "used_BW\t" << used_BW << "\ttotal_BW\t" << total_BW << "\tratio\t" << ((double) used_BW / total_BW) << endl;	
		cout << endl;

		cout << "********** comparison group **********" << endl << endl;
		used_BW = 0;
		for (int i=0; i<SEDGENUM; i++) {
			used_BW += C_Topo->edges[i]->filled_weight;
		}
		cout << "used_BW\t" << used_BW << "\ttotal_BW\t" << total_BW << "\tratio\t" << ((double) used_BW / total_BW) << endl;
		cout << endl;
		*/	
	}	
	return 0;
}

void init_request(Request* request, C_Request* c_request, int n_request, bool accepted, double qos) {
	int node1;
	int node2;

	double _mean;
	double _stddev;
 	double _eps = qos;

	bool _checked = false;
	bool _used = false;	
	bool _accepted = accepted;

	double _demand;

	double _total_mean;
	double _total_var;
	double _total_value;
	double _total_demand;

	if (_accepted == true) {
		for (int i=0; i<n_request; i++) {
			node1 = rand() % SNODENUM;

			while (1) {
				node2 = rand() % SNODENUM;
				if (node1 != node2/**/)
					break;
			}
			
			_mean = double(rand() % 5 + 10);	
			
			_stddev = 2.0;	
			//_stddev = double(rand() % 3 + 1);	

			_demand = _mean + _stddev * normsinv(double(1.0 - _eps));

			request[i].Set_Nodes(node1, node2);
			request[i].Set_Mean(_mean);
			request[i].Set_Var(_stddev * _stddev);
			request[i].Set_Epsilon(_eps);

			request[i].Set_Checked(_checked);
			request[i].Set_Used(_used);
			request[i].Set_Accepted(_accepted);

			c_request[i].Set_Nodes(node1, node2);
			c_request[i].Set_Demand(_demand);
			c_request[i].Set_Accepted(_accepted);	
		}
	}
	else {		
		for (int i=0; i<n_request; i++) {
			_total_mean = 0;
			_total_var = 0;
			_total_value = 0;
			_total_demand = 0;
	
			int f_count;

			if (relax_debug == true) {
				f_count = 1;
			}
			else {
				f_count = rand() % MAX_FLOW_SIZE + 1;	// the number of flows per each request	
			}
	
			request[i].flow_count = f_count;
			request[i].SetFlowTable(f_count);	
			c_request[i].c_flow_count = f_count;
			c_request[i].SetCFlowTable(f_count);

			for (int j=0; j<f_count; j++) {
				if (relax_debug == true) {
					node1 = 3;
					node2 = 7;
				
					_mean = double(rand() % 5 + 6);	
					_stddev = double(rand() % 3 + 1);	
					_demand = _mean + _stddev * normsinv(double(1.0 - _eps));

					_total_mean += _mean;
					_total_var += (_stddev * _stddev);
					_total_demand += _demand;
					
					request[i].MakeFlows(j, node1, node2, _mean, _stddev, _eps);
					c_request[i].MakeCFlows(j, node1, node2, _demand);
				}
				else {
					node1 = rand() % SNODENUM;

					while (1) {
						node2 = rand() % SNODENUM;
						if (node1 != node2/**/)
							break;
					}
					_mean = double(rand() % 5 + 6);	
					_stddev = double(rand() % 3 + 1);	
					_demand = _mean + _stddev * normsinv(double(1.0 - _eps));

					_total_mean += _mean;
					_total_var += (_stddev * _stddev);
					_total_demand += _demand;

					request[i].MakeFlows(j, node1, node2, _mean, _stddev, _eps);
					c_request[i].MakeCFlows(j, node1, node2, _demand);
				}
			}
			_total_value = _total_mean + sqrt(_total_var) * normsinv(double(1.0 - _eps));		
		
			request[i].Set_Total_Mean(_total_mean);
			request[i].Set_Total_Var(_total_var);
			request[i].Set_Total_Value(_total_value);
			request[i].Set_Accepted(_accepted);

			c_request[i].Set_Total_Demand(_total_demand);
			c_request[i].Set_Accepted(_accepted);	
		}
	}
}

void find_k_shortest_paths(Request request, int top_k, ostream& out_stream, ostream& out_stream2) {
	Graph my_graph("B4_SN");
	vector<BasePath*> result;
	vector<BasePath*>::iterator result_iter;

	int src = request.Get_Source();
	int dst = request.Get_Destination();

	YenTopKShortestPathsAlg yenAlg(my_graph, my_graph.get_vertex(src), my_graph.get_vertex(dst));
	yenAlg.get_shortest_paths(my_graph.get_vertex(src), my_graph.get_vertex(dst), top_k, result);  
	
	// Trace
	for (result_iter=result.begin(); result_iter!=result.end(); result_iter++) {
		//(*result_iter)->PrintOut(cout);
		(*result_iter)->PrintOut(out_stream);	
		(*result_iter)->PrintOut(out_stream2);	
	}
}

void find_k_shortest_paths_for_new_requests(Request request, int flow_number, int top_k, ostream& out_stream) {
	Graph my_graph("B4_SN");
	vector<BasePath*> result;
	vector<BasePath*>::iterator result_iter;

	int src = request.flows[flow_number]->src2;
	int dst = request.flows[flow_number]->dst2;

	YenTopKShortestPathsAlg yenAlg(my_graph, my_graph.get_vertex(src), my_graph.get_vertex(dst));
	yenAlg.get_shortest_paths(my_graph.get_vertex(src), my_graph.get_vertex(dst), top_k, result);  
	
	// Trace
	for (result_iter=result.begin(); result_iter!=result.end(); result_iter++) {
		//(*result_iter)->PrintOut(cout);
		(*result_iter)->PrintOut(out_stream);	
	}
}

void find_k_shortest_paths_for_c_new_requests(C_Request request, int flow_number, int top_k, ostream& out_stream) {
	Graph my_graph("B4_SN");
	vector<BasePath*> result;
	vector<BasePath*>::iterator result_iter;

	int src = request.c_flows[flow_number]->src2;
	int dst = request.c_flows[flow_number]->dst2;

	YenTopKShortestPathsAlg yenAlg(my_graph, my_graph.get_vertex(src), my_graph.get_vertex(dst));
	yenAlg.get_shortest_paths(my_graph.get_vertex(src), my_graph.get_vertex(dst), top_k, result);  
	
	// Trace
	for (result_iter=result.begin(); result_iter!=result.end(); result_iter++) {
		//(*result_iter)->PrintOut(cout);
		(*result_iter)->PrintOut(out_stream);	
	}
}

void ChangePathform(Request* embedded_req, vector <vector <int> >& embedded_user_path_vec, Phy_Topo* _topo) {
	vector <vector <int> >::iterator user_iter;
	vector <int>::iterator path_iter;

	int user_count = 0;
	int tmp_node1 = 0;
	int tmp_node2 = 0;

	int ID;
	double _mean;
	double _var;
	double _eps;

	for (user_iter = embedded_user_path_vec.begin(); user_iter != embedded_user_path_vec.end(); user_iter++) {
		for (path_iter = user_iter->begin(); path_iter != user_iter->end()-1; path_iter++) {
			tmp_node1 = *path_iter;
			tmp_node2 = *(path_iter + 1);

			for (int i=0; i<SEDGENUM; i++) {
				if ((tmp_node1 == _topo->edges[i]->node1 && tmp_node2 == _topo->edges[i]->node2)
						|| (tmp_node1 == _topo->edges[i]->node2 && tmp_node2 == _topo->edges[i]->node1)) {
					ID = embedded_req[user_count].request_ID;
					_mean = embedded_req[user_count].mean;
					_var = embedded_req[user_count].var;
					_eps = embedded_req[user_count].epsilon;
					
					_topo->edges[i]->SaveEmbeddedInfo(ID, _mean, _var, _eps);	// embedding			
				}
			}
		}
		user_count++;	
	}
}

void C_ChangePathform(C_Request* embedded_req, vector <vector <int> >& embedded_user_path_vec, C_Phy_Topo* _topo) {
	vector <vector <int> >::iterator user_iter;
	vector <int>::iterator path_iter;

	int user_count = 0;
	int tmp_node1 = 0;
	int tmp_node2 = 0;

	int ID;
	double _demand;

	for (user_iter = embedded_user_path_vec.begin(); user_iter != embedded_user_path_vec.end(); user_iter++) {
		for (path_iter = user_iter->begin(); path_iter != user_iter->end()-1; path_iter++) {
			tmp_node1 = *path_iter;
			tmp_node2 = *(path_iter + 1);
			
			for (int i=0; i<SEDGENUM; i++) {
				if ((tmp_node1 == _topo->edges[i]->node1 && tmp_node2 == _topo->edges[i]->node2)
						|| (tmp_node1 == _topo->edges[i]->node2 && tmp_node2 == _topo->edges[i]->node1)) {
					ID = embedded_req[user_count].request_ID;
					_demand = embedded_req[user_count].demand;
					
					_topo->edges[i]->SaveEmbeddedInfo(ID, _demand);	// embedding			
				}
			}
		}
		user_count++;	
	}
}

vector <int> ChangeNewRequestPathsform(Request* new_req, vector <int>& user_path_vec, Phy_Topo* _topo) {
	vector <int> path;	
	vector <int>::iterator path_iter;

	int tmp_node1 = 0;
	int tmp_node2 = 0;

	for (path_iter = user_path_vec.begin(); path_iter != user_path_vec.end()-1; path_iter++) {
		tmp_node1 = *path_iter;
		tmp_node2 = *(path_iter + 1);
	
		for (int i=0; i<SEDGENUM; i++) {
			if ((tmp_node1 == _topo->edges[i]->node1 && tmp_node2 == _topo->edges[i]->node2)
					|| (tmp_node1 == _topo->edges[i]->node2 && tmp_node2 == _topo->edges[i]->node1)) {
					path.push_back(_topo->edges[i]->edge_ID);
			}
		}
	}
	return path;
}

vector <int> C_ChangeNewRequestPathsform(C_Request* new_req, vector <int>& user_path_vec, C_Phy_Topo* _topo) {
	vector <int> path;	
	vector <int>::iterator path_iter;

	int tmp_node1 = 0;
	int tmp_node2 = 0;

	for (path_iter = user_path_vec.begin(); path_iter != user_path_vec.end()-1; path_iter++) {
		tmp_node1 = *path_iter;
		tmp_node2 = *(path_iter + 1);
	
		for (int i=0; i<SEDGENUM; i++) {
			if ((tmp_node1 == _topo->edges[i]->node1 && tmp_node2 == _topo->edges[i]->node2)
					|| (tmp_node1 == _topo->edges[i]->node2 && tmp_node2 == _topo->edges[i]->node1)) {
					path.push_back(_topo->edges[i]->edge_ID);
			}
		}
	}
	return path;
}

void testDijkstraGraph()
{
	Graph* my_graph_pt = new Graph("B4_SN");
	DijkstraShortestPathAlg shortest_path_alg(my_graph_pt);
	BasePath* result =
		shortest_path_alg.get_shortest_path(
			my_graph_pt->get_vertex(0), my_graph_pt->get_vertex(9));
	result->PrintOut(cout);
}

void testYenAlg()
{
	Graph my_graph("B4_SN");

	YenTopKShortestPathsAlg yenAlg(my_graph, my_graph.get_vertex(0),
		my_graph.get_vertex(9));

	int i=0;
	while(yenAlg.has_next())
	{
		++i;
		cout << "path " << i << ": " << endl;	
		yenAlg.next()->PrintOut(cout);
	}
}
