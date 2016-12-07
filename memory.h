#pragma once 

#include<string>
#include<iostream>
#include<vector>
using namespace std;
template <class T>

class ObjectPool
{
public:
	struct ObjectNode
	{
		ObjectNode *_next;
		void *_mem;				//内存块
		size_t _objectNum;	   //内存块中的对象数目
		ObjectNode(size_t objectSize,size_t objectNum = 32)
			:_objectNum(objectNum)
			, _next(NULL)
		{
			_mem = malloc(objectSize* _objectNum);
		}
		~ObjectNode()
		{
			if (_mem != NULL)
			{
				delete[] _mem;
			}
		}
	};

public:
	ObjectPool()
		:_maxNum(100000)
		, _lastDelete(NULL)
		, _countIn(0)
	{
		_first = _last = new ObjectNode(_objectSize);
	}

	T* New()
	{
		/*
		1.优先使用释放的空间
		2.使用已开辟的空间的剩余部分
		3.从新开辟节点，
		*/
		if (_lastDelete != NULL)
		{
			T *ret = _lastDelete;
			_lastDelete = *((T**)_lastDelete);
			return new(ret)T;
		}						
		else if (_countIn >= _last->_objectNum-1)
		{
			_AllocateNewNode();
		}

		//2.3 类似   ：找到合适的位置初始化
		char *address = (char*)_last->_mem;
		address += _objectSize*_countIn;
		T *result = new(address)T();
		_countIn++;
		return  result;		  //return new((T*)address)T();
	}

	void Delete(T* ptr)
	{
		if (ptr != NULL)
		{
			*(T**)ptr = _lastDelete;//二级指针是用来记录一级指针的地址，二级指针本身代表它自己存储区或里面的数据。
			_lastDelete = ptr;
		}
	}

	~ObjectPool()
	{
		if (_first != NULL)
		{
			while (_first)
			{
				ObjectNode *del = _first;
				_first = _first->_next;
				delete del;
			}
			_first = _last = NULL;
		}
	}		  
protected:
	static	size_t _ObjectSize()
	{
		if (sizeof(T) > sizeof(void *))
			return sizeof(T);
		else
			return sizeof(void *);
	}

	void _AllocateNewNode()
	{
		if (_last->_objectNum >= _maxNum)
		{
			ObjectNode *cur = _last;
			_last = new ObjectNode(_objectSize, _maxNum);
			cur->_next = _last;
			_countIn = 0;

		}
		else
		{
			ObjectNode *cur = _last;
			_objectnum = _last->_objectNum * 2;
			_last = new ObjectNode(_objectSize, _objectnum);
			cur->_next = _last;
			_countIn = 0;
		}
	}
protected:
	ObjectNode *_first;
	ObjectNode *_last;
	size_t _maxNum;
	static size_t _objectSize;
	size_t _objectnum;
	T *_lastDelete;
	size_t _countIn;	   //对应的内存块中已申请对象数目，

};
template<class T>
size_t ObjectPool<T>::_objectSize = ObjectPool<T>::_ObjectSize();

///////////////////////////////////////////////////////////
// 测试内存对象池的常规使用和内存对象的重复使用
void TestObjectPool()
{
	vector<string*> v;

	ObjectPool<string> pool;
	for (size_t i = 0; i < 32; ++i)
	{
		v.push_back(pool.New());
		printf("Pool New [%d]: %p\n", i, v.back());
	}

	while (!v.empty())
	{
		pool.Delete(v.back());
		v.pop_back();
	}

	for (size_t i = 0; i < 32; ++i)
	{
		v.push_back(pool.New());
		printf("Pool New [%d]: %p\n", i, v.back());
	}

	v.push_back(pool.New());
}

#include <Windows.h>

// 针对当前的内存对象池进行简单的性能测试
void TestObjectPoolOP()
{
	size_t begin, end;
	vector<string*> v;
	const size_t N = 1000000;
	v.reserve(N);

	cout << "pool new/delete===============================" << endl;
	// 反复申请释放5次
	begin = GetTickCount();
	ObjectPool<string> pool;
	for (size_t i = 0; i < N; ++i)
	{
		v.push_back(pool.New());
	}

	while (!v.empty())
	{
		pool.Delete(v.back());
		v.pop_back();
	}

	for (size_t i = 0; i < N; ++i)
	{
		v.push_back(pool.New());
	}

	while (!v.empty())
	{
		pool.Delete(v.back());
		v.pop_back();
	}

	for (size_t i = 0; i < N; ++i)
	{
		v.push_back(pool.New());
	}

	while (!v.empty())
	{
		pool.Delete(v.back());
		v.pop_back();
	}

	for (size_t i = 0; i < N; ++i)
	{
		v.push_back(pool.New());
	}

	while (!v.empty())
	{
		pool.Delete(v.back());
		v.pop_back();
	}

	for (size_t i = 0; i < N; ++i)
	{
		v.push_back(pool.New());
	}

	while (!v.empty())
	{
		pool.Delete(v.back());
		v.pop_back();
	}


	end = GetTickCount();
	cout << "Pool:" << end - begin << endl;

	cout << "new/delete===============================" << endl;
	begin = GetTickCount();

	for (size_t i = 0; i < N; ++i)
	{
		v.push_back(new string);
	}

	while (!v.empty())
	{
		delete v.back();
		v.pop_back();
	}

	for (size_t i = 0; i < N; ++i)
	{
		v.push_back(new string);
	}

	while (!v.empty())
	{
		delete v.back();
		v.pop_back();
	}

	for (size_t i = 0; i < N; ++i)
	{
		v.push_back(new string);
	}

	while (!v.empty())
	{
		delete v.back();
		v.pop_back();
	}

	for (size_t i = 0; i < N; ++i)
	{
		v.push_back(new string);
	}

	while (!v.empty())
	{
		delete v.back();
		v.pop_back();
	}

	for (size_t i = 0; i < N; ++i)
	{
		v.push_back(new string);
	}

	while (!v.empty())
	{
		delete v.back();
		v.pop_back();
	}

	end = GetTickCount();
	cout << "new/delete:" << end - begin << endl;
}
