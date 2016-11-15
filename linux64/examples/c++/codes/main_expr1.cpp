#include "gurobi_c++.h"

#include <iostream>
#include <stdlib.h>		// for rand
#include <time.h>		// for srand
#include <string.h>		// for strtok_r
#include <sstream>
#include <functional>	// for sorting
#include <cmath>

// for k-shortest paths
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

// for k-shortest paths
#include "GraphElements.h"
#include "Graph.h"
#include "DijkstraShortestPathAlg.h"
#include "YenTopKShortestPathsAlg.h"

#define SNODENUM 4	// number of datacenters: 4 
#define SEDGENUM 5 	// number of edges: 5 
#define CAPACITY 45	// capacity: 90Mbps
#define MAX_PATH_SIZE 10	// all paths could not consist of more than 10 edges
#define MAX_FLOW_SIZE 3		// the maximum number of flows per each request

#define CAPACITY_LIMIT 150	// cn(l)/C-c(l)

using namespace std;

// initialization of static variable in Request Class
int Request::count = 0;
int C_Request::c_count = 0;

// function declaration
void init_request(Request*, C_Request*, int, bool, double);
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

enum Cost {
	Construction,
	Borrow
};

int main(int argc, char* argv[]) {
	srand(atoi(argv[1]));	// seed change

	double req_qos = atof(argv[2]);	// epsilon

	// linear approximation value for each epsilon
	double approximation_slope = atof(argv[3]);
	double approximation_constant = atof(argv[4]);
	double expansion_point = atof(argv[5]);

	double scailing_constant = atof(argv[6]);	// scailing for the cost to provisioning

	int n_request = 30;	// the number of new requests
	int top_k = 3;	// consider only top_k shortest paths

	// for experiments
	int remaining_servers[SNODENUM];	// the number of idle servers in each datacenter
	int c_remaining_servers[SNODENUM];
	for (int i=0; i<SNODENUM; i++) {
		remaining_servers[i] = 3;
		c_remaining_servers[i] = 3;
	}

	char node_arr_for_path[100];	// store node_path to change link_path (temporary)

	bool generalized_ver = false;	// true: consider profitability, false: AC (LP3)
	bool check_profitability = false;	// true: LP2, false: LP1	

	enum Alg alg = Strong_Var;
	enum Cost cost = Borrow;

	vector <int>::iterator path_iter;

	ofstream acc_req_result_File;
	ofstream c_acc_req_result_File;
	ofstream embedded_path_File;
	acc_req_result_File.open("data/accepted_requests", ios_base::out | ios_base::app);
	c_acc_req_result_File.open("data/c_accepted_requests", ios_base::out | ios_base::app);
	embedded_path_File.open("data/embedded_paths", ios_base::out | ios_base::app);

	ofstream provisioning_result_File;
	ofstream c_provisioning_result_File;
	provisioning_result_File.open("data/provisioning_result", ios_base::out | ios_base::app);
	c_provisioning_result_File.open("data/c_provisioning_result", ios_base::out | ios_base::app);

	ofstream sub_provisioning_result_File;
	ofstream c_sub_provisioning_result_File;
	sub_provisioning_result_File.open("data/sub_provisioning_result", ios_base::out | ios_base::app);
	c_sub_provisioning_result_File.open("data/c_sub_provisioning_result", ios_base::out | ios_base::app);

	// for experiments
	ofstream expr_path_File("/home/cloud1/IoT/input1.txt");
	ofstream expr_demand_File("/home/cloud1/IoT/input_mean1.txt");
	ofstream c_expr_path_File("/home/cloud1/IoT/c_input1.txt");
	ofstream c_expr_demand_File("/home/cloud1/IoT/c_input_mean1.txt");

	embedded_path_File << "======" << atoi(argv[1]) << "th trial=====" << endl;	

	// initialization of substrate network
	Phy_Topo *Topo;
	Topo = new Phy_Topo(SNODENUM, SEDGENUM, CAPACITY);

	// initialization of new substrate network for the comparison group
	C_Phy_Topo *C_Topo;
	C_Topo = new C_Phy_Topo(SNODENUM, SEDGENUM, CAPACITY);

	// ======================================== request initialization ========================================

	/* initialization of new requests */
	Request request[n_request];	
	C_Request c_request[n_request];	
	init_request(request, c_request, n_request, false, req_qos);

	// Sort requests as descending order (reference: total demand)
	sort(request, request + n_request);
	sort(c_request, c_request + n_request);

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

	vector <int> c_new_req_path_vec[n_request][MAX_FLOW_SIZE][top_k];
	ofstream out_c_new_File("environment/c_new_requests_path.txt");	

	// for experiments
	vector <int> path_node_vec[n_request][MAX_FLOW_SIZE][top_k];
	vector <int> c_path_node_vec[n_request][MAX_FLOW_SIZE][top_k];
	vector <int> demand_vec[n_request][MAX_FLOW_SIZE];
	vector <int> c_demand_vec[n_request][MAX_FLOW_SIZE];

	for (int i=0; i<n_request; i++) {
		out_new_File << "new_request[" << i << "]" << endl;	
		out_c_new_File << "new_request[" << i << "]" << endl;

		int flow_number = request[i].flow_count;
		int c_flow_number = c_request[i].c_flow_count;

		for (int j=0; j<flow_number; j++) {
			out_new_File << "[" << i << "][" << j << "]\tsrc\t" << request[i].flows[j]->src2 << "\tdst\t" << request[i].flows[j]->dst2 << "\tmean\t"
				<< request[i].flows[j]->mean2 << "\tvar\t" << request[i].flows[j]->var2 << "\tepsilon\t" << request[i].flows[j]->epsilon2 << endl;
			find_k_shortest_paths_for_new_requests(request[i], j, top_k, out_new_File);

			out_c_new_File << "[" << i << "][" << j << "]\tsrc\t" << c_request[i].c_flows[j]->src2 << "\tdst\t" << c_request[i].c_flows[j]->dst2 << "\tdemand\t"
				<< c_request[i].c_flows[j]->demand2 << endl;
			find_k_shortest_paths_for_c_new_requests(c_request[i], j, top_k, out_c_new_File);
		}
	}
	out_new_File << "the end" << endl;	
	out_new_File.close();	
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

			// for experiments
			path_node_vec[user_count-1][flow_count-1][path_count-1] = test_vector;

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

			// for experiments
			c_path_node_vec[user_count-1][flow_count-1][path_count-1] = test_vector;

			c_new_req_path_vec[user_count-1][flow_count-1][path_count-1] = C_ChangeNewRequestPathsform(c_request, test_vector, C_Topo);	// change path form 	
		}
	}

	/* update link_indicator matrix */
	for (int i=0; i<n_request; i++) {
		for (int j=0; j<MAX_FLOW_SIZE; j++) {
			for (int k=0; k<top_k; k++) {
				for (path_iter = new_req_path_vec[i][j][k].begin(); path_iter != new_req_path_vec[i][j][k].end(); path_iter++) {
					link_indicator[i][j][k][*(path_iter)] = 1; 
				}
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
				for (path_iter = path_node_vec[i][j][k].begin(); path_iter != path_node_vec[i][j][k].end(); path_iter++) {
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

	/* ======================================== LP1: Generalized Version (our model, minimize cost, greedy, gurobi) ======================================== */

	if (generalized_ver == true && check_profitability == false) {
		for (int i=0; i<n_request; i++) {
			int flow_number = request[i].flow_count;	
			int test_value = 0;

			// for experiments: check server availability 
			for (int j=0; j<flow_number; j++) {
				if (remaining_servers[request[i].flows[j]->src2] == 0 || remaining_servers[request[i].flows[j]->dst2] == 0) {
					test_value++;	
					break;
				}
			}

			if (test_value != 0)
				continue;

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

				// create variables 1: path_allocator E_i,pm (1 if a request is embedded on the path, 0 otherwise)
				GRBVar path_allocator[MAX_FLOW_SIZE][top_k];		
				flow_number = request[i].flow_count;	
				for (int j=0; j<MAX_FLOW_SIZE; j++) {
					for (int k=0; k<top_k; k++) {
						string s = "R" + itos(i) + "F" + itos(j) + "P" + itos(k);
						path_allocator[j][k] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, s);
					}
				}

				// create variables 2: provisioning_indicator c_n(l), 18 
				GRBVar provisioning_indicator[SEDGENUM];
				for (int l=0; l<SEDGENUM; l++) {
					string s = "L" + itos(l);
					provisioning_indicator[l] = model.addVar(0.0, 100.0, 0.0, GRB_INTEGER, s); 
				}

				// intergrate variables into model
				model.update();	

				// set objective: minimize total flow
				GRBLinExpr obj = 0;	
				for (int j=0; j<MAX_FLOW_SIZE; j++) {
					for (int k=0; k<top_k; k++) {
						for (int l=0; l<SEDGENUM; l++) {
							if (j<flow_number)	
								obj += request[i].flows[j]->mean2 * path_allocator[j][k] * link_indicator[i][j][k][l];
							else{
								// do nothing
							}
						}
					}
				}

				// set objective: minimize provisioning bandwidth	
				switch (cost) {
					case Construction:
						for (int l=0; l<SEDGENUM; l++) {
							obj += scailing_constant * provisioning_indicator[l];
						}
						break;

					case Borrow:
						for (int l=0; l<SEDGENUM; l++) {
							obj += scailing_constant * provisioning_indicator[l]/(CAPACITY_LIMIT - Topo->edges[l]->weight);
						}
						break;

					default:
						break;
				}
				model.setObjective(obj, GRB_MINIMIZE);	

				// set constraint 1: each flow must use only 1 path (path_allocator E_i,pm) 
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

				// set constraint 2: link constraints with approximation (path_allocator E_i,pm, provisioning_indicator c_n(l))
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
								}	// for path: k
							}	// for flow: j

							// linear approximation
							//link_constraint_expr = (0.120406) - (0.189989) * (((link_weight - link_constraint_expr_mean) / sqrt(constraint_expr_var)) - 1.281);
							link_constraint_expr = (approximation_constant) - (approximation_slope) * (((link_weight - link_constraint_expr_mean) / sqrt(constraint_expr_var)) - expansion_point);
							string s = "LINK_CONSTRAINT_L" + itos(l);
							model.addConstr(link_constraint_expr <= link_min_eps, s);		
						}
						break;

					case Strong_Var:
						for (int l=0; l<SEDGENUM; l++) {
							GRBLinExpr link_constraint_expr;	// link_constraint_expr <= min_eps

							GRBLinExpr variable_path = Topo->edges[l]->filled_weight;	// rifht side of link_constraint_expr
							GRBLinExpr variable_provisioning = Topo->edges[l]->weight + provisioning_indicator[l];	// left side of link_constraint_expr

							double constraint_expr_var = Topo->edges[l]->total_var;

							double link_min_eps = Topo->edges[l]->min_epsilon;

							for (int j=0; j<flow_number; j++) {
								for (int k=0; k<top_k; k++) {
									variable_path += request[i].flows[j]->mean2 * path_allocator[j][k] * link_indicator[i][j][k][l];

									if (link_indicator[i][j][k][l] == 1) {	// variance should be reflected on the link l
										for (path_iter = new_req_path_vec[i][j][k].begin(); path_iter != new_req_path_vec[i][j][k].end(); path_iter++) {
											if (l == *(path_iter)) {
												if (var_added_indicator_for_strong_var[j][l] == 0) {
													constraint_expr_var += request[i].flows[j]->var2;
													var_added_indicator_for_strong_var[j][l] = 1;
													link_min_eps = min(Topo->edges[l]->min_epsilon, request[i].flows[j]->epsilon2);
												}
												else {
													// do nothing (already reflected)
												}
											}	
										}
									}
								}	// for path: k
							}	// for flow: j

							// have to be modified
							if (constraint_expr_var == 0) {
								constraint_expr_var = 1.0;	
							}

							// linear approximation
							//link_constraint_expr = (0.120406) - (0.189989) * (((link_weight - link_constraint_expr_mean) / sqrt(constraint_expr_var)) - 1.281);
							link_constraint_expr = (approximation_constant) - (approximation_slope) * (((variable_provisioning - variable_path)
										/ sqrt(constraint_expr_var)) - expansion_point);
							string s = "LINK_CONSTRAINT_L" + itos(l);
							model.addConstr(link_constraint_expr <= link_min_eps, s);		
						}	// for edge: l
						break;

					default:
						break;
				}

				// set constraint 3: capacity limit (weight + provisioning bw <= capacity limit)
				GRBLinExpr cap_limit_expr;
				for (int l=0; l<SEDGENUM; l++) {
					cap_limit_expr = Topo->edges[l]->weight + provisioning_indicator[l];	
					string s = "CAP_LIMIT_L" + itos(l);
					model.addConstr(cap_limit_expr <= CAPACITY_LIMIT, s);
				}

				// optimize model
				model.optimize();

				// The following functions are called when the feasibility is satisfied
				// save reqeust information on each link
				for (int j=0; j<flow_number; j++) {
					for (int k=0; k<top_k; k++) {
						if (path_allocator[j][k].get(GRB_DoubleAttr_X) == 1) {	// the flow j should be embedded on path k
							for (path_iter = new_req_path_vec[i][j][k].begin(); path_iter != new_req_path_vec[i][j][k].end(); path_iter++) {
								int ID = i;

								double m = request[i].flows[j]->mean2;
								double v = request[i].flows[j]->var2;
								double e = request[i].flows[j]->epsilon2;

								Topo->edges[*path_iter]->SaveEmbeddedInfo(ID, m, v, e);
							}
							embedded_path_File << "ID: " << i << ", FLOW: " << j << ", Path: " << k << endl << endl;
						}
					}
				}

				// save provisionined bandwidth information on each link 
				for (int j=0; j<flow_number; j++) {
					for (int k=0; k<top_k; k++) {
						if (path_allocator[j][k].get(GRB_DoubleAttr_X) == 1) {	// the flow j should be embedded on path k
							for (path_iter = new_req_path_vec[i][j][k].begin(); path_iter != new_req_path_vec[i][j][k].end(); path_iter++) {
								if (provisioning_indicator[*path_iter].get(GRB_DoubleAttr_X) != 0) {
									Topo->edges[*path_iter]->SaveProvisioningInfo(provisioning_indicator[*path_iter].get(GRB_DoubleAttr_X));
								}
							}
						}
					}
				}

				// for experiments
				for (int j=0; j<flow_number; j++) {
					remaining_servers[request[i].flows[j]->src2]--;
					remaining_servers[request[i].flows[j]->dst2]--;

					for (int k=0; k<top_k; k++) {
						if (path_allocator[j][k].get(GRB_DoubleAttr_X) == 1) {	// the flow j should be embedded on path k
							for (path_iter = path_node_vec[i][j][k].begin(); path_iter != path_node_vec[i][j][k].end(); path_iter++) {
								expr_path_File << *(path_iter) + 1 << "\t";
							}
							expr_path_File << endl;
							expr_demand_File << request[i].flows[j]->mean2 << "\t" << sqrt(request[i].flows[j]->var2) << endl;
						}
					}
				}

				Topo->UpdateAllEdgeInfo();
			}	// try to find optimum value
			catch(GRBException e) {
				cout << "Error code = " << e.getErrorCode() << endl;
				cout << e.getMessage() << endl;
			}
			catch(...) {
				cout << "Exception during optimization" << endl;
			}	
		}	// for each request: i

		expr_path_File.close();
		expr_demand_File.close();
	}

	/* ======================================== LP2: Generalized Version with profitability check (our model, minimize cost, greedy, constraint 4 is added) ======================================== */

	else if (generalized_ver == true && check_profitability == true) {
		for (int i=0; i<n_request; i++) {
			int flow_number = request[i].flow_count;	
			int test_value = 0;

			// for experiments: check server availability 
			for (int j=0; j<flow_number; j++) {
				if (remaining_servers[request[i].flows[j]->src2] == 0 || remaining_servers[request[i].flows[j]->dst2] == 0) {
					test_value++;	
					break;
				}
			}

			if (test_value != 0)
				continue;

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

				// create variables 1: path_allocator E_i,pm (1 if a request is embedded on the path, 0 otherwise)
				GRBVar path_allocator[MAX_FLOW_SIZE][top_k];		
				for (int j=0; j<MAX_FLOW_SIZE; j++) {
					for (int k=0; k<top_k; k++) {
						string s = "R" + itos(i) + "F" + itos(j) + "P" + itos(k);
						path_allocator[j][k] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, s);
					}
				}

				// create variables 2: provisioning_indicator c_n(l), 18 
				GRBVar provisioning_indicator[SEDGENUM];
				for (int l=0; l<SEDGENUM; l++) {
					string s = "L" + itos(l);
					provisioning_indicator[l] = model.addVar(0.0, 100.0, 0.0, GRB_INTEGER, s); 
				}

				// intergrate variables into model
				model.update();	

				// set objective: minimize total flow 
				GRBLinExpr obj = 0;	
				for (int j=0; j<flow_number; j++) {
					double temp_mean = request[i].flows[j]->mean2;
					double temp_var = request[i].flows[j]->var2;
					double temp_eps = request[i].flows[j]->epsilon2;

					obj += temp_mean + sqrt(temp_var) * normsinv(double(1.0 - temp_eps));	
				}

				for (int j=0; j<MAX_FLOW_SIZE; j++) {
					for (int k=0; k<top_k; k++) {
						for (int l=0; l<SEDGENUM; l++) {
							if (j<flow_number)	
								obj -= request[i].flows[j]->mean2 * path_allocator[j][k] * link_indicator[i][j][k][l];
							else{
								// do nothing
							}
						}
					}
				}

				switch (cost) {
					case Construction:
						for (int l=0; l<SEDGENUM; l++) {
							obj -= scailing_constant * provisioning_indicator[l];
						}
						break;

					case Borrow:
						for (int l=0; l<SEDGENUM; l++) {
							obj -= scailing_constant * provisioning_indicator[l]/(CAPACITY_LIMIT - Topo->edges[l]->weight);
						}
						break;

					default:
						break;
				}
				model.setObjective(obj, GRB_MAXIMIZE);	

				// set constraint 1: each flow must use only 1 path (path_allocator E_i,pm) 
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

				// set constraint 2: link constraints with approximation (path_allocator E_i,pm, provisioning_indicator c_n(l))
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
								}	// for path: k
							}	// for flow: j

							// linear approximation
							//link_constraint_expr = (0.120406) - (0.189989) * (((link_weight - link_constraint_expr_mean) / sqrt(constraint_expr_var)) - 1.281);
							link_constraint_expr = (approximation_constant) - (approximation_slope) * (((link_weight - link_constraint_expr_mean) / sqrt(constraint_expr_var)) - expansion_point);
							string s = "LINK_CONSTRAINT_L" + itos(l);
							model.addConstr(link_constraint_expr <= link_min_eps, s);		
						}
						break;

					case Strong_Var:
						for (int l=0; l<SEDGENUM; l++) {
							GRBLinExpr link_constraint_expr;	// link_constraint_expr <= min_eps

							GRBLinExpr variable_path = Topo->edges[l]->filled_weight;	// rifht side of link_constraint_expr
							GRBLinExpr variable_provisioning = Topo->edges[l]->weight + provisioning_indicator[l];	// left side of link_constraint_expr

							double constraint_expr_var = Topo->edges[l]->total_var;

							double link_min_eps = Topo->edges[l]->min_epsilon;

							for (int j=0; j<flow_number; j++) {
								for (int k=0; k<top_k; k++) {
									variable_path += request[i].flows[j]->mean2 * path_allocator[j][k] * link_indicator[i][j][k][l];

									if (link_indicator[i][j][k][l] == 1) {	// variance should be reflected on the link l
										for (path_iter = new_req_path_vec[i][j][k].begin(); path_iter != new_req_path_vec[i][j][k].end(); path_iter++) {
											if (l == *(path_iter)) {
												if (var_added_indicator_for_strong_var[j][l] == 0) {
													constraint_expr_var += request[i].flows[j]->var2;
													var_added_indicator_for_strong_var[j][l] = 1;
													link_min_eps = min(Topo->edges[l]->min_epsilon, request[i].flows[j]->epsilon2);
												}
												else {
													// do nothing (already reflected)
												}
											}	
										}
									}
								}	// for path: k
							}	// for flow: j

							// have to be modified
							if (constraint_expr_var == 0) {
								constraint_expr_var = 1.0;	
							}

							// linear approximation
							//link_constraint_expr = (0.120406) - (0.189989) * (((link_weight - link_constraint_expr_mean) / sqrt(constraint_expr_var)) - 1.281);
							link_constraint_expr = (approximation_constant) - (approximation_slope) * (((variable_provisioning - variable_path)
										/ sqrt(constraint_expr_var)) - expansion_point);
							string s = "LINK_CONSTRAINT_L" + itos(l);
							model.addConstr(link_constraint_expr <= link_min_eps, s);		
						}	// for edge: l
						break;

					default:
						break;
				}

				// set constraint 3: capacity limit (weight + provisioning bw <= capacity limit)
				GRBLinExpr cap_limit_expr;
				for (int l=0; l<SEDGENUM; l++) {
					cap_limit_expr = Topo->edges[l]->weight + provisioning_indicator[l];	
					string s = "CAP_LIMIT_L" + itos(l);
					model.addConstr(cap_limit_expr <= CAPACITY_LIMIT, s);
				}

				// set constraint 4: profitability check (revenue - cost >= 0) 
				GRBLinExpr profit_expr;
				for (int j=0; j<flow_number; j++) {
					double temp_mean = request[i].flows[j]->mean2;
					double temp_var = request[i].flows[j]->var2;
					double temp_eps = request[i].flows[j]->epsilon2;

					profit_expr += temp_mean + sqrt(temp_var) * normsinv(double(1.0 - temp_eps));	
				}

				for (int j=0; j<MAX_FLOW_SIZE; j++) {
					for (int k=0; k<top_k; k++) {
						for (int l=0; l<SEDGENUM; l++) {
							if (j<flow_number)	
								profit_expr -= request[i].flows[j]->mean2 * path_allocator[j][k] * link_indicator[i][j][k][l];
							else{
								// do nothing
							}
						}
					}
				}

				switch (cost) {
					case Construction:
						for (int l=0; l<SEDGENUM; l++) {
							profit_expr -= scailing_constant * provisioning_indicator[l];
						}
						break;

					case Borrow:
						for (int l=0; l<SEDGENUM; l++) {
							profit_expr -= scailing_constant * provisioning_indicator[l]/(CAPACITY_LIMIT - Topo->edges[l]->weight);
						}
						break;

					default:
						break;
				}
				string t = "PROFIT_COND";
				model.addConstr(profit_expr >= 0, t);

				// optimize model
				model.optimize();

				// save reqeust information on each link
				for (int j=0; j<flow_number; j++) {
					for (int k=0; k<top_k; k++) {
						if (path_allocator[j][k].get(GRB_DoubleAttr_X) == 1) {	// the flow j should be embedded on path k
							for (path_iter = new_req_path_vec[i][j][k].begin(); path_iter != new_req_path_vec[i][j][k].end(); path_iter++) {
								int ID = i;

								double m = request[i].flows[j]->mean2;
								double v = request[i].flows[j]->var2;
								double e = request[i].flows[j]->epsilon2;

								Topo->edges[*path_iter]->SaveEmbeddedInfo(ID, m, v, e);
							}
							embedded_path_File << "ID: " << i << ", FLOW: " << j << ", Path: " << k << endl << endl;
						}
					}
				}

				// save provisionined bandwidth information on each link 
				for (int j=0; j<flow_number; j++) {
					for (int k=0; k<top_k; k++) {
						if (path_allocator[j][k].get(GRB_DoubleAttr_X) == 1) {	// the flow j should be embedded on path k
							for (path_iter = new_req_path_vec[i][j][k].begin(); path_iter != new_req_path_vec[i][j][k].end(); path_iter++) {
								if (provisioning_indicator[*path_iter].get(GRB_DoubleAttr_X) != 0) {
									Topo->edges[*path_iter]->SaveProvisioningInfo(provisioning_indicator[*path_iter].get(GRB_DoubleAttr_X));
								}
							}
						}
					}
				}

				// for experiments
				for (int j=0; j<flow_number; j++) {
					remaining_servers[request[i].flows[j]->src2]--;
					remaining_servers[request[i].flows[j]->dst2]--;

					for (int k=0; k<top_k; k++) {
						if (path_allocator[j][k].get(GRB_DoubleAttr_X) == 1) {	// the flow j should be embedded on path k
							for (path_iter = path_node_vec[i][j][k].begin(); path_iter != path_node_vec[i][j][k].end(); path_iter++) {
								expr_path_File << *(path_iter) + 1 << "\t";
							}
							expr_path_File << endl;
							expr_demand_File << request[i].flows[j]->mean2 << "\t" << sqrt(request[i].flows[j]->var2) << endl;
						}
					}
				}

				Topo->UpdateAllEdgeInfo();
			}	// try to find optimum value
			catch(GRBException e) {
				cout << "Error code = " << e.getErrorCode() << endl;
				cout << e.getMessage() << endl;
			}
			catch(...) {
				cout << "Exception during optimization" << endl;
			}	
		}	// for each request: i
		
		expr_path_File.close();
		expr_demand_File.close();
	}

	/* ======================================== LP3: Admission Control (our model, minimize flow, greedy, gurobi) ======================================== */

	else {
		for (int i=0; i<n_request; i++) {
			int flow_number = request[i].flow_count;	
			int test_value = 0;

			// for experiments: check server availability 
			for (int j=0; j<flow_number; j++) {
				if (remaining_servers[request[i].flows[j]->src2] == 0 || remaining_servers[request[i].flows[j]->dst2] == 0) {
					test_value++;	
					break;
				}
			}

			if (test_value != 0) {
				continue;
			}

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

									Topo->edges[*path_iter]->SaveEmbeddedInfo(ID, m, v, e);
								}

								embedded_path_File << "ID: " << i << ", FLOW: " << j << ", Path: " << k << endl << endl;
							}
						}
					}
				}

				// for experiments
				for (int j=0; j<flow_number; j++) {
					remaining_servers[request[i].flows[j]->src2]--;
					remaining_servers[request[i].flows[j]->dst2]--;

					for (int k=0; k<top_k; k++) {
						if (path_allocator[j][k].get(GRB_DoubleAttr_X) == 1) {	// the flow j should be embedded on path k
							for (path_iter = path_node_vec[i][j][k].begin(); path_iter != path_node_vec[i][j][k].end(); path_iter++) {
								expr_path_File << *(path_iter) + 1 << "\t";
							}
							expr_path_File << endl;
							expr_demand_File << request[i].flows[j]->mean2 << "\t" << sqrt(request[i].flows[j]->var2) << endl;
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
		}	// FOR EACH REQUEST (I)
		
		expr_path_File.close();
		expr_demand_File.close();
	}

	cout << "============================================== OUR MODEL END ===================================================" << endl;
	cout << endl;

	/* ======================================== LP (comparison group, greedy) ======================================== */

	if (generalized_ver == true) {
		for (int i=0; i<n_request; i++) {
			int c_flow_number = c_request[i].c_flow_count;	
			int test_value = 0;

			// for experiments: check server availability 
			for (int j=0; j<c_flow_number; j++) {
				if (c_remaining_servers[c_request[i].c_flows[j]->src2] == 0 || c_remaining_servers[c_request[i].c_flows[j]->dst2] == 0) {
					test_value++;	
					break;
				}
			}

			if (test_value != 0) {
				c_expr_path_File << "~~~~~~~~~~ERROR~~~~~~~~~~" << endl;	
				continue;
			}
			c_expr_path_File << "WWWWWWWWWWWWWWWWWAAAAAAAAAAAAAAAARRRRRRRRRRRRRRNNNNNNNNNNNNnn" << endl;	

			try {
				// create the environment and the model (default)
				GRBEnv c_env = GRBEnv();
				GRBModel c_model = GRBModel(c_env);	

				// create variables 1: path_allocator (1 if a request is embedded on the path, 0 otherwise)
				GRBVar c_path_allocator[MAX_FLOW_SIZE][top_k];		
				for (int j=0; j<MAX_FLOW_SIZE; j++) {
					for (int k=0; k<top_k; k++) {
						string s = "C_R" + itos(i) + "F" + itos(j) + "P" + itos(k);
						c_path_allocator[j][k] = c_model.addVar(0.0, 1.0, 0.0, GRB_BINARY, s);
					}
				}

				// create variables 2: provisioning indicator
				GRBVar c_provisioning_indicator[SEDGENUM];
				for (int l=0; l<SEDGENUM; l++) {
					string s = "C_L" + itos(l);
					c_provisioning_indicator[l] = c_model.addVar(0.0, 100.0, 0.0, GRB_INTEGER, s); 
				}

				// intergrate variables into model
				c_model.update();	

				// set objective: minimize total flow
				GRBLinExpr c_obj = 0;	
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

				// set objective: minimize provisioning bandwidth	
				switch (cost) {
					case Construction:
						for (int l=0; l<SEDGENUM; l++) {
							c_obj += scailing_constant * c_provisioning_indicator[l];
						}
						break;

					case Borrow:
						for (int l=0; l<SEDGENUM; l++) {
							c_obj += scailing_constant * c_provisioning_indicator[l]/(CAPACITY_LIMIT - C_Topo->edges[l]->weight);
						}
						break;

					default:
						break;
				}
				c_model.setObjective(c_obj, GRB_MINIMIZE);	

				// set constraint 1: each flow must use only 1 path
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

				// set constraint 2: link constraints (path_allocator, c_provisioning_indicator) 
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
					c_model.addConstr(c_link_constraint_expr <= link_weight + c_provisioning_indicator[l], s);		
				}

				// set constraint 3: capacity limit (weight + provisioning bw <= capacity limit)
				GRBLinExpr c_cap_limit_expr;
				for (int l=0; l<SEDGENUM; l++) {
					c_cap_limit_expr = C_Topo->edges[l]->weight + c_provisioning_indicator[l];	
					string s = "C_CAP_LIMIT_L" + itos(l);
					c_model.addConstr(c_cap_limit_expr <= CAPACITY_LIMIT, s);
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

				// save provisioned bandwidth information on each link 
				for (int l=0; l<SEDGENUM; l++) {
					//					cout << "i\t" << i << "\tl\t" << l << "\tvalue\t" << c_provisioning_indicator[l].get(GRB_DoubleAttr_X) << endl;
					if (c_provisioning_indicator[l].get(GRB_DoubleAttr_X) != 0) {
						C_Topo->edges[l]->SaveProvisioningInfo(c_provisioning_indicator[l].get(GRB_DoubleAttr_X));
					}
				}

				// for experiments
				for (int j=0; j<c_flow_number; j++) {
					c_remaining_servers[c_request[i].c_flows[j]->src2]--;
					c_remaining_servers[c_request[i].c_flows[j]->dst2]--;
					
					for (int k=0; k<top_k; k++) {
						if (c_path_allocator[j][k].get(GRB_DoubleAttr_X) == 1) {	// the flow j should be embedded on path k
							for (path_iter = c_path_node_vec[i][j][k].begin(); path_iter != c_path_node_vec[i][j][k].end(); path_iter++) {
								c_expr_path_File << *(path_iter) + 1 << "\t";
							}
							c_expr_path_File << endl;
							c_expr_demand_File << c_request[i].c_flows[j]->mean2 << "\t" << sqrt(c_request[i].c_flows[j]->var2) << endl;
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
	}
	else {
		for (int i=0; i<n_request; i++) {
			int c_flow_number = c_request[i].c_flow_count;	
			int test_value = 0;

			// for experiments: check server availability 
			for (int j=0; j<c_flow_number; j++) {
				if (c_remaining_servers[c_request[i].c_flows[j]->src2] == 0 || c_remaining_servers[c_request[i].c_flows[j]->dst2] == 0) {
					test_value++;	
					break;
				}
			}

			if (test_value != 0) {
				continue;
			}
			
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

				// for experiments
				for (int j=0; j<c_flow_number; j++) {
					c_remaining_servers[c_request[i].c_flows[j]->src2]--;
					c_remaining_servers[c_request[i].c_flows[j]->dst2]--;
					
					for (int k=0; k<top_k; k++) {
						if (c_path_allocator[j][k].get(GRB_DoubleAttr_X) == 1) {	// the flow j should be embedded on path k
							for (path_iter = c_path_node_vec[i][j][k].begin(); path_iter != c_path_node_vec[i][j][k].end(); path_iter++) {
								c_expr_path_File << *(path_iter) + 1 << "\t";
							}
							c_expr_path_File << endl;
							c_expr_demand_File << c_request[i].c_flows[j]->mean2 << "\t" << sqrt(c_request[i].c_flows[j]->var2)  << endl;
						}
					}
				}

				C_Topo->UpdateAllEdgeInfo();
			}	// try (optimize)

			catch(GRBException e) {
				cout << "Error code = " << e.getErrorCode() << endl;
				cout << e.getMessage() << endl;
			} 
			catch(...) {
				cout << "Exception during optimization" << endl;
			}	
		}	// for each request (i)
	}

	c_expr_path_File.close();
	c_expr_demand_File.close();

	/* ======================================== Tracing ======================================== */

	Topo->PrintAllEdgeInfo(cout);

	cout << endl;	
	cout << "****************************** our model ******************************" << endl << endl;

	// accepted requests	
	int accepted_count = 0;	
	if (generalized_ver == false) {
		for (int i=0; i<n_request; i++) {
			if (request[i].accepted == true) {
				accepted_count++;
			}
		}
	}

	// accepted mean
	double accepted_mean = 0;
	double total_me = 0;
	if (generalized_ver == false) {
		for (int i=0; i<n_request; i++) {
			total_me = request[i].total_mean;
			if (request[i].accepted == true) {
				accepted_mean += request[i].total_mean;	
			}
		}
	}

	// utilization
	double total_filled = 0;	
	double total_weight = 0;
	double total_prov = 0;	
	if (generalized_ver == true) {
		for (int l=0; l<SEDGENUM; l++) {
			total_filled += Topo->edges[l]->filled_weight + sqrt(Topo->edges[l]->total_var) * normsinv(double(1.0 - Topo->edges[l]->min_epsilon));
			total_weight += Topo->edges[l]->weight;	
			total_prov += Topo->edges[l]->provisioned_weight;	

			sub_provisioning_result_File << "edgeID\t" << Topo->edges[l]->edge_ID << "\tweight\t" << Topo->edges[l]->weight << "\tprovisioned_bw\t" << Topo->edges[l]->provisioned_weight
				<< "\tmin_qos\t" << Topo->edges[l]->min_epsilon << endl;
		}
		cout << "filled_weight_total\t" << total_filled << "\tweight_total\t" << total_weight << "\tnormalized_utilization\t" << (total_filled/total_weight) << endl;
		cout << "provisioning_bw_total\t" << total_prov << endl;
	} else {
		for (int l=0; l<SEDGENUM; l++) {
			total_filled += Topo->edges[l]->filled_weight;
			total_weight += Topo->edges[l]->weight;	
		}
		cout << "# of new accepted requests\t" << accepted_count << "\ttotal_new_requests\t" << n_request << "\treq_qos\t" << req_qos << endl;
		cout << "accepted_mean\t" << accepted_mean << "\ttotal_filled\t" << total_filled << endl;	
		cout << "normalized_accepted_mean\t" << (accepted_mean/total_me) << endl;
	}

	// actual qos (feasibility) check
	double qos = 0;	
	double min_qos = 1.0;	
	double tmp_qos = 0;	
	bool violation_indicator = false;	
	for (int i=0; i<SEDGENUM; i++) {
		tmp_qos = Topo->edges[i]->GetActualQoS();
		min_qos = min(min_qos, tmp_qos);

		if (tmp_qos < (1-req_qos)) {
			violation_indicator = true;
		}
		else {
			//do nothing
		}
		qos += tmp_qos;
	}
	cout << "sum_qos\t" << qos << "\taverage_qos\t" << qos/(SEDGENUM) << "\tviolation_indicator\t" << violation_indicator << endl;

	acc_req_result_File << accepted_count << "\t" << n_request << "\t" << min_qos << "\t" << violation_indicator << "\t" << req_qos << "\t" << accepted_mean << "\t" << total_filled << endl;
	cout << accepted_count << "\t" << n_request << "\t" << min_qos << "\t" << violation_indicator << "\t" << req_qos << endl;
	//	acc_req_result_File << accepted_count << "\t" << n_request << endl;
	//Topo->PrintAllEdgeInfo(acc_req_result_File);	
	acc_req_result_File.close();	
	cout << endl << endl;

	provisioning_result_File << "scailing_constant\t" << scailing_constant << "\ttotal_prov\t" << total_prov << "\tutilization\t" << (total_filled/total_weight) << endl;
	provisioning_result_File.close();	
	sub_provisioning_result_File.close();	

	cout << "****************************** comparison group ******************************" << endl << endl;

	// c_utilization
	double c_total_filled = 0;	
	double c_total_weight = 0;
	double c_total_prov = 0;	
	if (generalized_ver == true) {
		for (int l=0; l<SEDGENUM; l++) {
			c_total_filled += C_Topo->edges[l]->filled_weight;
			c_total_weight += C_Topo->edges[l]->weight;	
			c_total_prov += C_Topo->edges[l]->provisioned_weight;	

			c_sub_provisioning_result_File << "edgeID\t" << C_Topo->edges[l]->edge_ID << "\tweight\t" << C_Topo->edges[l]->weight
				<< "\tprovisioned_bw\t" << C_Topo->edges[l]->provisioned_weight << endl;
		}
		cout << "c_total_filled\t" << c_total_filled << "\tc_total_weight\t" << c_total_weight << "\tc_normalized_utilization\t" << (c_total_filled/c_total_weight) << endl;
		cout << "c_provisioning_bw_total\t" << c_total_prov << endl;
	}		

	c_provisioning_result_File << "scailing_constant\t" << scailing_constant << "\tc_total_prov\t" << c_total_prov << "\tc_utilization\t" << (c_total_filled/c_total_weight) << endl;
	c_provisioning_result_File.close();	
	c_sub_provisioning_result_File.close();	
	/*	
		accepted_count = 0;	
		accepted_mean = 0;	
		for (int i=0; i<n_request; i++) {
		if (c_request[i].accepted == true) {
		accepted_count++;
		}
		}
		cout << "# of new accepted c_requests\t" << accepted_count << "\ttotal_new_requests\t" << n_request << endl;
		cout << "accepted_mean\t" << accepted_mean << endl;	
		c_acc_req_result_File << accepted_count << "\t" << n_request << "\t" << req_qos << endl;
	//	c_acc_req_result_File << accepted_count << "\t" << n_request << endl;
	c_acc_req_result_File.close();	
	embedded_path_File.close();	
	*/
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
			
			_mean = double(rand() % 5 + 5);	
			_stddev = double(rand() % 3 + 1);	
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
			
			// total traffic of the request (not the flow)	
			_total_mean = 0;
			_total_var = 0;
			_total_value = 0;
			_total_demand = 0;

			int f_count = 1;
//			int f_count = rand() % MAX_FLOW_SIZE + 1;	// the number of flows per each request	
	
			request[i].flow_count = f_count;
			request[i].SetFlowTable(f_count);	
			c_request[i].c_flow_count = f_count;
			c_request[i].SetCFlowTable(f_count);

			for (int j=0; j<f_count; j++) {
					node1 = rand() % SNODENUM;

					while (1) {
						node2 = rand() % SNODENUM;
						if (node1 != node2)
							break;
					}
				
					_mean = double(rand() %21 + 0);		// mean: 3~5	
					_stddev = double(rand() % 3 + 2);	// var: 3~4	
					_demand = _mean + _stddev * normsinv(double(1.0 - _eps));

					_total_mean += _mean;
					_total_var += (_stddev * _stddev);
					_total_value += _demand;	
					_total_demand += _demand;

					request[i].MakeFlows(j, node1, node2, _mean, _stddev, _eps);
					c_request[i].MakeCFlows(j, node1, node2, _mean, _stddev, _eps, _demand);
			}
//			_total_value = _total_mean + sqrt(_total_var) * normsinv(double(1.0 - _eps));		
		
			request[i].Set_Total_Mean(_total_mean);
			request[i].Set_Total_Var(_total_var);
			request[i].Set_Total_Value(_total_value);
			request[i].Set_Accepted(_accepted);

			c_request[i].Set_Total_Demand(_total_demand);
			c_request[i].Set_Accepted(_accepted);	
		}
	}
}

void find_k_shortest_paths_for_new_requests(Request request, int flow_number, int top_k, ostream& out_stream) {
	Graph my_graph("topo_expr1");
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
	Graph my_graph("topo_expr1");
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
	Graph* my_graph_pt = new Graph("topo_expr1");	
	DijkstraShortestPathAlg shortest_path_alg(my_graph_pt);
	BasePath* result =
		shortest_path_alg.get_shortest_path(
			my_graph_pt->get_vertex(0), my_graph_pt->get_vertex(9));
	result->PrintOut(cout);
}

void testYenAlg()
{
	Graph my_graph("topo_expr1");	
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
