/* C_Request.cpp */

#include <iostream>
#include "C_Request.h"

using namespace std;

/* C_Flow Class */
C_Flow::C_Flow() {

}

C_Flow::C_Flow(int node1, int node2, double _m, double _v, double _e, double _d) {
	src2 = node1;
	dst2 = node2;

	mean2 = _m;
	var2 = _v * _v;
	epsilon2 = _e;

	demand2 = _d;
}

C_Flow::~C_Flow() {

}

C_Request::C_Request() {
	request_ID = c_count++;
}

C_Request::~C_Request() {

}

int C_Request::Get_ID(void) {
	return request_ID;
}

int C_Request::Get_Source(void) {
	return src_node;
}

int C_Request::Get_Destination(void) {
	return dest_node;
}

double C_Request::Get_Demand(void) {
	return demand;
}
		
bool C_Request::Get_Checked(void) {
	return checked;
}

bool C_Request::Get_Used(void) {
	return used;
}

bool C_Request::Get_Accepted(void) {
	return accepted;
}

void C_Request::Set_Nodes(int node1, int node2) {
	src_node = node1;
	dest_node = node2;
}

void C_Request::Set_Demand(double _demand) {
	demand = _demand;
}

void C_Request::Set_Checked(bool _checked) {
	checked = _checked;
}

void C_Request::Set_Used(bool _used) {
	used = _used;
}

void C_Request::Set_Accepted(bool _accepted) {
	accepted = _accepted;
}

void C_Request::Set_Total_Demand(double _total_demand) {
	total_demand = _total_demand;
}

void C_Request::SetCFlowTable(int f_count) {
	c_flows = new C_Flow*[f_count];
}

void C_Request::MakeCFlows(int flow_number, int node1, int node2, double _m, double _v, double _e, double _d) {
	c_flows[flow_number] = new C_Flow(node1, node2, _m, _v, _e, _d);
}

void C_Request::PrintC_Request(void) {
	cout << "ID\t" << request_ID << "\tSrc\t" << src_node << "\tDest\t" << dest_node
		<< "\tDemand\t" << demand << endl;
}
