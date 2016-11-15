#include <iostream>
#include <stdlib.h>
#include "Normsinv.h"

using namespace std;

int main(int argc, char* argv[]) {
	double mean, stddev, eps;
	
	mean = atof(argv[1]);
	stddev = atof(argv[2]);
	eps = atof(argv[3]);

	/*
	mean = 5;
	stddev = 3;
	eps = 0.1;
*/

	cout << "mean\t" << mean << "\tstddev\t" << stddev << "\teps\t" << eps << endl;

	double demand = mean + stddev * normsinv(double(1.0 - eps));	

	cout << "demand\t" << demand << endl;

	return 0;
}
