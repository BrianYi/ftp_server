#pragma once
#include "ServerHeader.h"
#include "Packet.h"

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
	void pop()
	{
		std::unique_lock<std::mutex> lock( this->fQueueMx );
		fElemQueue.pop();
	}
	bool empty()
	{
		return fElemQueue.empty();
	}
private:
	std::queue<T> fElemQueue;
	std::mutex fQueueMx;
};

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
	void pop()
	{
		std::unique_lock<std::mutex> lock( this->fQueueMx );
		fElemQueue.pop();
	}
	bool empty()
	{
		return fElemQueue.empty();
	}
private:
	std::priority_queue<T, std::vector<T>, cmp<T>> fElemQueue;
	std::mutex fQueueMx;
};

