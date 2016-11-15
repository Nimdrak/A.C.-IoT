/* Request.h */

#ifndef REQUEST_H
#define REQUEST_H 

#include <vector>

using namespace std;

class Flow {
	public:
		int src2;
		int dst2;

		double mean2;
		double var2;
		double epsilon2;

	public:
		Flow();	
		Flow(int node1, int node2, double _m, double _v, double _e);
		~Flow();
};

class Request {
	public:
		static int count;
		int request_ID;
		
		int flow_count;

		int src_node;
		int dest_node;

		double mean;
		double var;
		double epsilon;

		double total_mean;
		double total_var;
		double min_epsilon;

		double total_value;

		bool checked;
		bool used;
		bool accepted;

		Flow** flows;		

		bool operator <(const Request &b) const {
			return this->total_value > b.total_value;
		}

	public:
		Request();
		~Request();

		int Get_ID(void);
		int Get_Source(void);
		int Get_Destination(void);
		double Get_Mean(void);
		double Get_Var(void);
		double Get_Epsilon(void);
		bool Get_Checked(void);
		bool Get_Used(void);
		bool Get_Accepted(void);

		void Set_Nodes(int node1, int node2);
		void Set_Mean(double _mean);
		void Set_Var(double _var);
		void Set_Epsilon(double _eps);
		void Set_Checked(bool _checked);
		void Set_Used(bool _used);
		void Set_Accepted(bool _accepted);

		void Set_Total_Mean(double _total_mean);
		void Set_Total_Var(double _total_var);
		void Set_Total_Value(double _total_value);

		void SetFlowTable(int f_count);
		void MakeFlows(int flow_number, int node1, int node2, double _m, double _v, double _e);

		void PrintRequest(void);
};

#endif
