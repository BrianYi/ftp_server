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
	T pop()
	{
		std::unique_lock<std::mutex> lock( this->fStackMx );
		T elem = fElemStack.top();
		fElemStack.pop();
		return elem;
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
