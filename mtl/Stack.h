/*
 * Stack.h
 *
 *  Created on: Mar 30, 2018
 *      Author: shafiknassar
 */

#ifndef MTL_STACK_H_
#define MTL_STACK_H_

#include <assert.h>

template <class T>
class Stack
{
	T*  arr;
	int head;
	int size;
public:
	Stack(int sz) : head(0), size(sz)
                          { arr = new T[size]; }
	~Stack()              { delete[] arr; }


	bool     isFull()     { return head == size-1; }
	bool     isEmpty()    { return head == 0; }
	T        peek()       { assert(head != 0); return arr[head]; }
	T        pop()        { assert(head != 0); return arr[head--]; }
	void     push(T e)    { assert(head < size-1); arr[++head] = e; }
};


#endif /* MTL_STACK_H_ */
