#include <iostream>
#include <fstream>
#include <stdlib.h>

using namespace std;

int main(int argc, char *argv[]) {
	cout << "the number of inputs: " << argc << endl;
	cout << "inputs" << endl;

	int a;
	int b = rand();

	for (int i=1; i<argc; i++) {
		cout << "argv[" << i << "]: " << argv[i] << endl;	
		a = atoi(argv[i]);	
	}
	cout << "a: " << a << endl;
	cout << "rand: " << b << endl;

	return 0;
}
