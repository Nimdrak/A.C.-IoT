#include <fstream>
#include <iostream>

using namespace std;

#define MAX_SIZE 100
char inputString[MAX_SIZE];

int main() {
	ifstream inFile("B4_SN_topology.txt");
	
	while(!inFile.eof()){
		inFile.getline(inputString, 100);
		cout << inputString << endl;	
	}

	inFile.close();
	return 0;
}
