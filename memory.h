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
		void *_mem;				//�ڴ��
		size_t _objectNum;	   //�ڴ���еĶ�����Ŀ
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
		1.����ʹ���ͷŵĿռ�
		2.ʹ���ѿ��ٵĿռ��ʣ�ಿ��
		3.���¿��ٽڵ㣬
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

		//2.3 ����   ���ҵ����ʵ�λ�ó�ʼ��
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
			*(T**)ptr = _lastDelete;//����ָ����������¼һ��ָ��ĵ�ַ������ָ�뱾��������Լ��洢������������ݡ�
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
	size_t _countIn;	   //��Ӧ���ڴ���������������Ŀ��

};
template<class T>
size_t ObjectPool<T>::_objectSize = ObjectPool<T>::_ObjectSize();

///////////////////////////////////////////////////////////
// �����ڴ����صĳ���ʹ�ú��ڴ������ظ�ʹ��
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

// ��Ե�ǰ���ڴ����ؽ��м򵥵����ܲ���
void TestObjectPoolOP()
{
	size_t begin, end;
	vector<string*> v;
	const size_t N = 1000000;
	v.reserve(N);

	cout << "pool new/delete===============================" << endl;
	// ���������ͷ�5��
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
