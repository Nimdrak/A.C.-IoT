/* C_Request.h */

#ifndef C_REQUEST_INCLUDED
#define C_REQUEST_INCLUDED

#include <vector>

using namespace std;

class C_Flow {
	public:	
		int src2;
		int dst2;

		double demand2;

		double mean2;
		double var2;
		double epsilon2;

	public:
		C_Flow();	
		C_Flow(int node1, int node2, double _m, double _v, double _e, double _d);
		~C_Flow();
};

class C_Request {
	public:
		static int c_count;
		int request_ID;

		int c_flow_count;

		int src_node;
		int dest_node;

		double demand;

		double total_demand;

		double total_mean;
		double total_var;
		double min_epsilon;

		bool checked;
		bool used;
		bool accepted;

		C_Flow** c_flows;

		bool operator <(const C_Request &b) const {
			return this->total_demand > b.total_demand;
		}
	
	public:
		C_Request();
		~C_Request();

		int Get_ID(void);
		int Get_Source(void);
		int Get_Destination(void);
		double Get_Demand(void);
		bool Get_Checked(void);
		bool Get_Used(void);
		bool Get_Accepted(void);

		void Set_Nodes(int node1, int node2);
		void Set_Demand(double _demand);
		void Set_Checked(bool _checked);
		void Set_Used(bool _used);
		void Set_Accepted(bool _accepted);

		void Set_Total_Demand(double _total_demand);

		void SetCFlowTable(int f_count);
		void MakeCFlows(int flow_number, int node1, int node2, double _m, double _v, double _e, double _d);
		
		void PrintC_Request(void);
};

#endif 
