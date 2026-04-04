#include  <iostream>
#include  <cstdlib>
#include  <climits>
#include "tree.h"
#include "stack.h"

using namespace std;

BST::BST(int s) {
	n = s;
	A = new char[n];
	for(int i = 0; i < n; i++) 
		A[i] = '_';	
}

BST::~BST() {
	delete[] A;
}

void BST :: insert(char c) {
	int i = 0;
	if(A[i] == '_') {
		A[i] = c;
		return;
	}
	
	while(i < n) {
		if(A[i] == c) {
			return;
		}
		if(A[i] < c) {
			i = 2 * i + 2;
			if(A[i] == '_') {
				A[i] = c;
				return;
			} 
		}
		else {
			i = 2 * i + 1;
			if(A[i] == '_') {
				A[i] = c;
				return;
			}
		
		}
	}
}

void BST :: search(char data, int i) {
	if(i >= n) {
        cout << "Element Not Found\n";
		return;
	}
	if(A[i] == data) {
		cout << "Element is Found at index " << i << "\n";
		return;
	}
	if(A[i] < data) 
		search(data, 2*i + 2);
	else  
		search(data, 2*i + 1);
}

void BST :: display() {
	for(int i = 0; i < n; i++) {
		cout << A[i] << " ";
	}
	cout << endl;
	return;
}

void BST :: preorder(int i) {
	if(i >= n || A[i] == '_') return;
	cout << A[i] << " " ;
	preorder(2*i + 1);
	preorder(2*i + 2);
}

void BST :: inorder(int i) {
	if(i >= n || A[i] == '_') return;
	inorder(2*i + 1);
	cout << A[i] << " " ;
	inorder(2*i + 2);
}

void BST :: postorder(int i) {
	if(i >= n || A[i] == '_') return;
	postorder(2*i + 1);
	postorder(2*i + 2);
	cout << A[i] << " " ;
}

void BST :: inorder_iterative() {
    stack<int> st;
    int i = 0;
    while(i < n || !st.isempty()) {
        if(i < n && A[i] != '_') {
            st.push(i);
            i = 2 * i + 1;
        }
        else {
            i = st.peek();
            st.pop();
            cout << A[i] << " ";
            i = 2 * i + 2;
        }
    }
}

void BST :: preorder_iterative() {
    int i = 0;
    stack<int> st;

    while(i < n || !st.isempty()) {
        if(i < n && A[i] != '_') {
            cout << A[i] << " ";
            st.push(i);
            i = 2 * i + 1;
        }
        else {
            i = st.peek();
            st.pop();
            i = 2 * i + 2;
        }
    }
}