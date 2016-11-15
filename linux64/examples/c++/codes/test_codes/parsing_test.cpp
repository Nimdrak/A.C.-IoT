#include <string.h>
#include <fstream>
#include <iostream>

#define MAX_SIZE 1000
#define DELIM "\t\n"

char inputString[MAX_SIZE];

using namespace std;

int main() {
	ifstream inFile("B4_SN_topology.txt");
	
	while(!inFile.eof()){
		inFile.getline(inputString, 100);

		char* token;
		char* current;
		char* buf = inputString;	

		//cout << "buf: " << buf << endl;

		int count = 0;

		while ((token = strtok_r(buf, DELIM, &current)) != NULL) {
			cout << "token: " << token << "\tcount: " << count << endl;	
			buf = NULL;	
			count++;	
		}
	}

	inFile.close();
	return 0;
}
