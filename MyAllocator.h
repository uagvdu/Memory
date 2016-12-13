#pragma once 
#include<iostream>

/*�ڴ�أ����������void*���͵Ŀռ䣬�������ǿ������ת�����ֲ��Ǵ�������Ҫʲô�������ͣ���lastdelete���ͷ�ʱ���ָ�룬����Ҫ�ٴ���⡣*/
using namespace std;

// Trace ����
// 

#define __DEBUG__

FILE* fOut = fopen("trace.log", "w");

static string GetFileName(const string& path)
{
	char ch = '/';

#ifdef _WIN32
	ch = '\\';
#endif

	size_t pos = path.rfind(ch);
	if (pos == string::npos)
		return path;
	else
		return path.substr(pos + 1);
}
// ���ڵ���׷�ݵ�trace log
inline static void __trace_debug(const char* function,
	const char * filename, int line, char* format, ...)
{
	// ��ȡ�����ļ�
#ifdef __DEBUG__
	// ������ú�������Ϣ
	fprintf(stdout, "�� %s:%d��%s", GetFileName(filename).c_str(), line, function);
	fprintf(fOut, "�� %s:%d��%s", GetFileName(filename).c_str(), line, function);

	// ����û����trace��Ϣ
	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	vfprintf(fOut, format, args);
	va_end(args);
#endif
}

#define __TRACE_DEBUG(...)  \
	__trace_debug(__FUNCTION__, __FILE__, __LINE__, __VA_ARGS__);


template <int inst>
class __Malloc_Alloc_Template //һ��������
{
	typedef void(*ALLOC_HANDLER)();//����һ�����������
	static ALLOC_HANDLER __mallocAllocOomHandler;

	static void* OOM_Malloc(size_t n) //STL ��59ҳ
	{
		void* ret;
		while (1)
		{
			if (__mallocAllocOomHandler == NULL)//�����������������ڣ�����ϵͳһ��ռ䶼����ʣ�࣬��ôҲ��ֻ�����쳣
			{
				throw bad_alloc();
			}
			else
			{
				//�����ͷ��ڴ�
				(*__mallocAllocOomHandler)();//���ú��������ڴ棬���غ����ͷ��ڴ棬һֱѭ����ȥ��˵�����ͻ��к��ʵĿռ䱻�ͷŴӶ����ٳ�����Ҫ�Ŀռ�
				ret = malloc(n);
				if (ret)
				{
					return ret;
				}
			}
		}
	}

public://�����ڴ�
	static void* Allocate(size_t n)
	{
		 __TRACE_DEBUG("һ���ռ�����������%uBytes\n", n)
		void *ret = malloc(n);
		if (ret == 0)
			ret = OOM_Malloc(n);//ϵͳ�����ڴ�ʧ��
		return ret;
	}

	static void deallocate(void *ptr,size_t n)	
	{
		 __TRACE_DEBUG("һ���ռ��������ͷ�0x%p,%uBytes\n",ptr, n);
		free(ptr);
		ptr = NULL;
	}

	static ALLOC_HANDLER SetMallocAllocHandler(ALLOC_HANDLER f)// void(*ALLOC_HANDLER)() f????
	{
		ALLOC_HANDLER old = __mallocAllocOomHandler;
		__mallocAllocOomHandler = f;
		return old;
	}
};

template<int inst>
typename __Malloc_Alloc_Template<inst>::ALLOC_HANDLER __Malloc_Alloc_Template<inst>::__mallocAllocOomHandler = 0;


////////
/*�����ռ�������*/

template<bool threads,int inst>
class Deafult_Malloc_Template
{
public:

	enum{ __ALIGN = 8 };// ��׼ֵ
	enum{ __MAX = 128 };	//��������ֽ���;
	enum{ __BLOCKNUM = __MAX / __ALIGN}; //�������
protected:
	static size_t FreeListIndex(size_t bytes)
	{
		return (((bytes)+__ALIGN - 1) / __ALIGN - 1);
	}

	static size_t RoundUp(size_t bytes)
	{
		return (((bytes)+__ALIGN - 1) & ~(__ALIGN - 1));
	}
	static void* Chunk(size_t& nobjs,size_t bytes)
	{//bytes :��������������С

		size_t total_bytes = nobjs * bytes;
		size_t bytes_left = (char*)_end_free - (char*)_start_free;//�ڴ��ʣ���ֽ���
		char *ret = NULL;
		__TRACE_DEBUG("�ڴ��ʣ��%ubytes,��Ҫ����%uBytes\n", bytes_left, total_bytes);

		if (bytes_left >= total_bytes)
		{
			__TRACE_DEBUG("�ڴ����㹻%u�������С�ڴ�\n", nobjs);

			ret =  _start_free;
			 _start_free += total_bytes;
			return ret;
		}
		else if(bytes_left >= bytes)//ʣ���ֽ���Ϊ1 - 20
		{
			
			nobjs = bytes_left / bytes;
			total_bytes = nobjs*bytes;
			__TRACE_DEBUG("�ڴ�ֻ�ܹ�����%u�������С�ڴ�\n", nobjs);
			ret = _start_free;
			 _start_free += total_bytes;
			return ret;
		}
		else//�ڴ����һ������Ĵ�С�������ֵ�ʱ��,��ô����Ҫ���¿��ٴ���ڴ�
		{
			size_t newSize = 2 * total_bytes + RoundUp(_heap_size >>4);
			if (bytes_left > 0)//�������ڴ���еĲ�����ͷ�������ü�ֵ:ʣ��Ŀռ�϶���8�ı���
			{
				int index = FreeListIndex(bytes_left);
				Obj*  listIndex= _freelist[index];		//ͷ�� ,���Ż�
				((Obj*)_start_free)->_freelistlink = listIndex;
				_freelist[index] = ((Obj*)_start_free);
			}

			_start_free = (char*)malloc(newSize);//�ڴ�أ�
			if (_start_free == 0)
			{//mallocʧ��
				/*
				1�����ż�����������ӵ�еĶ�����
				����Ѱ��Ӧ��������ʵ������飬���ʵ�����δʹ�õ����飬���棺ָ����緵��һ���ϴ������
				*/
				for (int i = bytes ; i < __MAX; i+=__ALIGN)
				{
					int index = FreeListIndex(i);
					if (_freelist[index])
					{
						Obj* dst = _freelist[index];
						_freelist[index] = _freelist[index]->_freelistlink;
						 _start_free =(char*) dst;
						 _end_free = (char*)((Obj*)_start_free + 1);
						return (Chunk(nobjs, bytes));
					}
				}
				_end_free = 0;//�ʼ���ٴ���ʱ��start_free��end_free ��Ϊ0������ʱ��Ϊ�㣬���´ο��ٴ���ڴ�ص�ʱ�򣬾Ͳ������bytes_left == 0�����
				_start_free = (char*)__Malloc_Alloc_Template<0>::Allocate (bytes);
			}
			(char*)_end_free = (char*)_start_free + newSize;
			_heap_size += newSize;
			return Chunk(nobjs,bytes);
		}

	}
	static void* refill(size_t bytes)//��Ӧ������������������
	{
		//�����䣿
		

		size_t nobjs = 20;//���20��������_freelist[index]�·�
		size_t totalbytes = nobjs*bytes;
		size_t index = FreeListIndex(bytes);
		void *chunk = Chunk(nobjs, bytes);
		if (nobjs == 1)
		{
			return  chunk;
		}

		else
		{
			__TRACE_DEBUG("��ʣ����ڴ��ҵ�����������:%d\n", nobjs - 1);
			Obj* tmp = _freelist[index];

			_freelist[index] = tmp = (Obj*)((char*)chunk + bytes );
			for (int i= 1;; i++)
			{

				if (nobjs - 1 == i)//��鿪ʼ��20������ռ䣬����tmp��ret������������ʱʣʮ�˸����ʶ�ѭ������Ϊ18�ξ�ook
				{
					//��i = 19ʱ��
					tmp->_freelistlink = 0;
					break;
				}
				else
				{
					tmp->_freelistlink = (Obj*)((char*)chunk + (i+1)*bytes);//��Ϊǰ����Obj* ��ǿ������ת�������Եõ��Ŀռ��С��sizeof(Obj)
					tmp = tmp->_freelistlink;
				}
			}
			//���ش���еĵ�һ��obj��С�Ķ���ռ䣬ʣ�µĴ��ռ���������������·�

			return (Obj*)chunk;//ȷ��λ���Լ������С�� ��Ȼ����ֵ��void*���ͣ�����ָ����������Ĵ�С�ǹ̶��ģ��������Ϊ�䶳����������鱻��������ת����ʹ��
		}
		/* void ���ܸ���void���͸�ֵ������void���Ϳ��Ը�void���͸�ֵ */

	}
public:
	static void* Allocate(size_t bytes)
	{
		__TRACE_DEBUG("�����ռ�����������%uBytes\n", bytes);
		if (bytes > __BLOCKNUM)				//����128����һ���ռ����������ٿռ�
			return (__Malloc_Alloc_Template<inst>::Allocate(bytes));

		size_t index = FreeListIndex(bytes);
		void *myfreelist = _freelist[index];

		if (myfreelist == NULL)						//���ҵ����������Ǹ�λ��û�п�������ʱ���������ռ�
			return refill(bytes);
		//	return _freelist[index];//��ȷ���ÿռ���ڵ�ʱ�򣬾���Ҫ���ÿռ������ȥ�����ҽ����������ŵ�ָ��ָ����һλ ��������������Ļ���һ����֪���Ŀ�ռ�δ��ʹ�ã������п������ͬһ��ռ䱻�ظ�ʹ��

		__TRACE_DEBUG("�����������%d��λ��ȡ�ڴ��\n", index);
		_freelist[index] = _freelist[index]->_freelistlink;				//�ҵ��ˣ��򷵻ظô�λ�ã���������ָ����һ���ռ�ĵ�ַ
		return myfreelist;

	}

	static void Deallocate(void* ptr,size_t n)
	{
		__TRACE_DEBUG("�����ռ��������ͷ�0x%p,%uBytes\n", ptr, n);

		if (n > __MAX)
		{
			__Malloc_Alloc_Template<0>::deallocate(ptr,n);
			return;
		}
		else
		{
			Obj* ret = (Obj*)ptr;
			size_t index = FreeListIndex(n);
			ret ->_freelistlink = _freelist[index];
			_freelist[index] =ret;

		}
	}

	
protected:
	
	union Obj
	{
		union Obj* _freelistlink;
		char _clientDate[1];
	};
	static Obj* _freelist[__BLOCKNUM];

	// Chunk allocation state.
	static char* _start_free;
	static char *_end_free;
	static size_t _heap_size;
};

template <bool threads, int inst>
typename Deafult_Malloc_Template<threads, inst>::Obj*  Deafult_Malloc_Template<threads, inst>::_freelist[__BLOCKNUM] = { 0 };

template <bool threads, int inst>
char* Deafult_Malloc_Template<threads, inst>::_start_free = 0;	// �ڲ��ڴ�ؿ�ʼ

template <bool threads, int inst>
char* Deafult_Malloc_Template<threads, inst>::_end_free = 0;		// �ڲ��ڴ�ؽ���

template <bool threads, int inst>
size_t Deafult_Malloc_Template<threads, inst>::_heap_size = 0;	// �������ڴ��Ĵ�С

void Test1()
{
	vector<pair<void*, size_t>> v; //˳����ڲ���Ա��һ��pair���͵Ķ���
	v.push_back(make_pair(Deafult_Malloc_Template<false, 0>::Allocate(129), 129));//����make_pair()������һ��pair���͵Ķ���

	for (size_t i = 0; i < 21; ++i)
	{
		v.push_back(make_pair(Deafult_Malloc_Template<false, 0>::Allocate(12), 12));
	}

	while (!v.empty())
	{
		Deafult_Malloc_Template<false, 0>::Deallocate(v.back().first, v.back().second);
		v.pop_back();
	}

	for (size_t i = 0; i < 21; ++i)
	{
		v.push_back(make_pair(Deafult_Malloc_Template<false, 0>::Allocate(7), 7));
	}

	for (size_t i = 0; i < 10; ++i)
	{
		v.push_back(make_pair(Deafult_Malloc_Template<false, 0>::Allocate(16), 16));
	}

	while (!v.empty())
	{
		Deafult_Malloc_Template<false, 0>::Deallocate(v.back().first, v.back().second);
		v.pop_back();
	}
}
//
//void Test3()
//{
//	cout << "����ϵͳ���ڴ�ľ�" << endl;
//
//	SimpleAlloc<char, Alloc>::Allocate(1024 * 1024 * 1024);
//	SimpleAlloc<char, Alloc>::Allocate(1024 * 1024);
//
//	//���ò��ԣ�˵��ϵͳ����С���ڴ���������Ǻ�ǿ�ġ�
//	for (int i = 0; i< 100000; ++i)
//	{
//		char*p1 = SimpleAlloc<char, Alloc>::Allocate(128);
//	}
//
//	cout << "�������" << endl;
//}
