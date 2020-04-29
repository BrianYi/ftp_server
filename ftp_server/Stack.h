#pragma once
#include "ServerHeader.h"
#include <stack>

template <class T>
class Stack
{
public:
	void push( T elem )
	{
		std::unique_lock<std::mutex> lock( this->fStackMx );
		fElemStack.push( elem );
	}
	T top()
	{
		return fElemStack.top();
	}
	void pop()
	{
		std::unique_lock<std::mutex> lock( this->fStackMx );
		fElemStack.pop();
	}
	bool empty()
	{
		return fElemStack.empty();
	}
	size_t size()
	{
		return fElemStack.size();
	}
private:
	std::stack<T> fElemStack;
	std::mutex fStackMx;
};
