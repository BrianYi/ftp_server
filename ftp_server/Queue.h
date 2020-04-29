#pragma once
#include "ServerHeader.h"

template <class T>
class cmp
{
public:
	bool operator()( T elem1, T elem2 )
	{
		if ( elem1->timestamp() > elem2->timestamp() )
			return true;
		return false;
	}
};

/*
 * multi-thread safe
 */
template <class T>
class Queue
{
public:
	void push( T elem )
	{
		std::unique_lock<std::mutex> lock( this->fQueueMx );
		fElemQueue.push( elem );
	}
	T front()
	{
		return fElemQueue.front();
	}
	T back()
	{
		return fElemQueue.back();
	}
	void pop()
	{
		std::unique_lock<std::mutex> lock( this->fQueueMx );
		fElemQueue.pop();
	}
	bool empty()
	{
		return fElemQueue.empty();
	}
	size_t size()
	{
		return fElemQueue.size();
	}
private:
	std::queue<T> fElemQueue;
	std::mutex fQueueMx;
};

/*
 * multi-thread safe
 */
template <class T>
class PriorityQueue
{
public:
	void push( T elem )
	{
		std::unique_lock<std::mutex> lock( this->fQueueMx );
		fElemQueue.push( elem );
	}
	T front()
	{
		return fElemQueue.top();
	}
	T back()
	{
		return fElemQueue.back();
	}
	void pop()
	{
		std::unique_lock<std::mutex> lock( this->fQueueMx );
		fElemQueue.pop();
	}
	bool empty()
	{
		return fElemQueue.empty();
	}
	size_t size()
	{
		return fElemQueue.size();
	}
private:
	std::priority_queue<T, std::vector<T>, cmp<T>> fElemQueue;
	std::mutex fQueueMx;
};

