#include <iostream>
#include <stdlib.h>
#include <algorithm>
#include <functional>

using namespace std;

struct edge {
	int a;
	int b;	
	int c;

	bool operator <(const edge &b) const {
		return this->a < b.a;
	}
};


int main(void) {
	edge e[10];
	
	for (int i=0; i<10; i++) {
		e[i].a = rand() % 1000;
		e[i].b = rand() % 100;
		e[i].c = rand() % 10;
		cout << "e[" << i << "]\ta\t" << e[i].a << "\tb\t" << e[i].b << "\tc\t" << e[i].c << endl;
	}

//	sort(e, e+10);

	cout << endl;
	cout << "AFTER" << endl;
	
	for (int i=0; i<10; i++) {
		cout << "e[" << i << "]\ta\t" << e[i].a << "\tb\t" << e[i].b << "\tc\t" << e[i].c << endl;
	}

	return 0;
}
