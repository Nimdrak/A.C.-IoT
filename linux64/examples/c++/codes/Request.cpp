/* Request.cpp */

#include <iostream>
#include "Request.h"

using namespace std;

/* Flow Class */
Flow::Flow() {

}

Flow::Flow(int node1, int node2, double _m, double _v, double _e) {
	src2 = node1;
	dst2 = node2;

	mean2 = _m;
	var2 = _v * _v;
	epsilon2 = _e;
}

Flow::~Flow() {

}

Request::Request() {
	request_ID = count++;
}

Request::~Request() {

}

int Request::Get_ID(void) {
	return request_ID;
}

int Request::Get_Source(void) {
	return src_node;
}

int Request::Get_Destination(void) {
	return dest_node;
}

double Request::Get_Mean(void) {
	return mean;
}

double Request::Get_Var(void) {
	return var;
}

double Request::Get_Epsilon(void) {
	return epsilon;
}

bool Request::Get_Checked(void) {
	return checked;
}

bool Request::Get_Used(void) {
	return used;
}

bool Request::Get_Accepted(void) {
	return accepted;
}

void Request::Set_Nodes(int node1, int node2) {
	src_node = node1;
	dest_node = node2;
}

void Request::Set_Mean(double _mean) {
	mean = _mean;
}

void Request::Set_Var(double _var) {
	var = _var;
}

void Request::Set_Epsilon(double _eps) {
	epsilon = _eps;
}

void Request::Set_Checked(bool _checked) {
	checked = _checked;
}

void Request::Set_Used(bool _used) {
	used = _used;
}

void Request::Set_Accepted(bool _accepted) {
	accepted = _accepted;
}

void Request::Set_Total_Mean(double _total_mean) {
	total_mean = _total_mean;
}

void Request::Set_Total_Var(double _total_var) {
	total_var = _total_var;
}

void Request::Set_Total_Value(double _total_value) {
	total_value = _total_value;
}


void Request::SetFlowTable(int f_count) {
	flows = new Flow*[f_count];
}

void Request::MakeFlows(int flow_number, int node1, int node2, double _m, double _v, double _e) {
	flows[flow_number] = new Flow(node1, node2, _m, _v, _e);
}

void Request::PrintRequest(void) {
	cout << "ID\t" << request_ID << "\tSrc\t" << src_node << "\tDest\t" << dest_node
		<< "\tMean\t" << mean << "\tVar\t" << var << "\tEpsilon\t" << epsilon << endl;
}
