#pragma once 
#include<iostream>

/*内存池，给对象分配void*类型的空间，分配后在强制类型转换，又不是创建对象，要什么数据类型，，lastdelete是释放时候的指针，还需要再次理解。*/
using namespace std;

// Trace 跟踪
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
// 用于调试追溯的trace log
inline static void __trace_debug(const char* function,
	const char * filename, int line, char* format, ...)
{
	// 读取配置文件
#ifdef __DEBUG__
	// 输出调用函数的信息
	fprintf(stdout, "【 %s:%d】%s", GetFileName(filename).c_str(), line, function);
	fprintf(fOut, "【 %s:%d】%s", GetFileName(filename).c_str(), line, function);

	// 输出用户打的trace信息
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
class __Malloc_Alloc_Template //一级配置器
{
	typedef void(*ALLOC_HANDLER)();//定义一个函数句柄，
	static ALLOC_HANDLER __mallocAllocOomHandler;

	static void* OOM_Malloc(size_t n) //STL 第59页
	{
		void* ret;
		while (1)
		{
			if (__mallocAllocOomHandler == NULL)//如果这个函数都不存在，代表系统一点空间都不会剩余，那么也就只能抛异常
			{
				throw bad_alloc();
			}
			else
			{
				//期望释放内存
				(*__mallocAllocOomHandler)();//调用函数消耗内存，返回函数释放内存，一直循环下去，说不定就会有合适的空间被释放从而开辟出来需要的空间
				ret = malloc(n);
				if (ret)
				{
					return ret;
				}
			}
		}
	}

public://开辟内存
	static void* Allocate(size_t n)
	{
		 __TRACE_DEBUG("一级空间配置器申请%uBytes\n", n)
		void *ret = malloc(n);
		if (ret == 0)
			ret = OOM_Malloc(n);//系统开辟内存失败
		return ret;
	}

	static void deallocate(void *ptr,size_t n)	
	{
		 __TRACE_DEBUG("一级空间配置器释放0x%p,%uBytes\n",ptr, n);
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
/*二级空间配置器*/

template<bool threads,int inst>
class Deafult_Malloc_Template
{
public:

	enum{ __ALIGN = 8 };// 基准值
	enum{ __MAX = 128 };	//区块最大字节数;
	enum{ __BLOCKNUM = __MAX / __ALIGN}; //区块个数
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
	{//bytes :自由链表的区块大小

		size_t total_bytes = nobjs * bytes;
		size_t bytes_left = (char*)_end_free - (char*)_start_free;//内存池剩余字节数
		char *ret = NULL;
		__TRACE_DEBUG("内存池剩余%ubytes,需要申请%uBytes\n", bytes_left, total_bytes);

		if (bytes_left >= total_bytes)
		{
			__TRACE_DEBUG("内存有足够%u个对象大小内存\n", nobjs);

			ret =  _start_free;
			 _start_free += total_bytes;
			return ret;
		}
		else if(bytes_left >= bytes)//剩余字节数为1 - 20
		{
			
			nobjs = bytes_left / bytes;
			total_bytes = nobjs*bytes;
			__TRACE_DEBUG("内存只能够分配%u个对象大小内存\n", nobjs);
			ret = _start_free;
			 _start_free += total_bytes;
			return ret;
		}
		else//内存池连一个区块的大小都不够分的时候,那么就需要重新开辟大块内存
		{
			size_t newSize = 2 * total_bytes + RoundUp(_heap_size >>4);
			if (bytes_left > 0)//试着让内存池中的残余零头还有利用价值:剩余的空间肯定是8的倍数
			{
				int index = FreeListIndex(bytes_left);
				Obj*  listIndex= _freelist[index];		//头插 ,防优化
				((Obj*)_start_free)->_freelistlink = listIndex;
				_freelist[index] = ((Obj*)_start_free);
			}

			_start_free = (char*)malloc(newSize);//内存池，
			if (_start_free == 0)
			{//malloc失败
				/*
				1，试着检视我们手上拥有的东西，
				，搜寻对应区块后面适当的区块，：适当：尚未使用的区块，后面：指给外界返回一个较大的区块
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
				_end_free = 0;//最开始开辟大块的时候，start_free和end_free 都为0，若此时不为零，则下次开辟大块内存池的时候，就不会出现bytes_left == 0的情况
				_start_free = (char*)__Malloc_Alloc_Template<0>::Allocate (bytes);
			}
			(char*)_end_free = (char*)_start_free + newSize;
			_heap_size += newSize;
			return Chunk(nobjs,bytes);
		}

	}
	static void* refill(size_t bytes)//对应的自由链表下面填充块
	{
		//如何填充？
		

		size_t nobjs = 20;//填充20个对象在_freelist[index]下方
		size_t totalbytes = nobjs*bytes;
		size_t index = FreeListIndex(bytes);
		void *chunk = Chunk(nobjs, bytes);
		if (nobjs == 1)
		{
			return  chunk;
		}

		else
		{
			__TRACE_DEBUG("将剩余的内存块挂到自由链表上:%d\n", nobjs - 1);
			Obj* tmp = _freelist[index];

			_freelist[index] = tmp = (Obj*)((char*)chunk + bytes );
			for (int i= 1;; i++)
			{

				if (nobjs - 1 == i)//大块开始是20个对象空间，后来tmp和ret用了两个，此时剩十八个，故而循环次数为18次就ook
				{
					//当i = 19时，
					tmp->_freelistlink = 0;
					break;
				}
				else
				{
					tmp->_freelistlink = (Obj*)((char*)chunk + (i+1)*bytes);//因为前面有Obj* 的强制类型转换，所以得到的空间大小是sizeof(Obj)
					tmp = tmp->_freelistlink;
				}
			}
			//返回大块中的第一个obj大小的对象空间，剩下的大块空间链在自由链表的下方

			return (Obj*)chunk;//确定位置以及区块大小， 虽然返回值是void*类型，但其指向的这个区块的大小是固定的，可以理解为冷冻，等这块区块被重新类型转换再使用
		}
		/* void 不能给非void类型赋值，但非void类型可以给void类型赋值 */

	}
public:
	static void* Allocate(size_t bytes)
	{
		__TRACE_DEBUG("二级空间配置器申请%uBytes\n", bytes);
		if (bytes > __BLOCKNUM)				//大于128则用一级空间配置器开辟空间
			return (__Malloc_Alloc_Template<inst>::Allocate(bytes));

		size_t index = FreeListIndex(bytes);
		void *myfreelist = _freelist[index];

		if (myfreelist == NULL)						//所找的自由链表那个位置没有可用区块时，重新填充空间
			return refill(bytes);
		//	return _freelist[index];//当确定该空间存在的时候，就需要将该空间给传过去，并且将自由链表存放的指针指向下一位 ：如果不这样做的话，一，不知道哪块空间未被使用，二，有可能造成同一块空间被重复使用

		__TRACE_DEBUG("在自由链表第%d个位置取内存块\n", index);
		_freelist[index] = _freelist[index]->_freelistlink;				//找到了，则返回该处位置，自由链表指向下一个空间的地址
		return myfreelist;

	}

	static void Deallocate(void* ptr,size_t n)
	{
		__TRACE_DEBUG("二级空间配置器释放0x%p,%uBytes\n", ptr, n);

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
char* Deafult_Malloc_Template<threads, inst>::_start_free = 0;	// 内部内存池开始

template <bool threads, int inst>
char* Deafult_Malloc_Template<threads, inst>::_end_free = 0;		// 内部内存池结束

template <bool threads, int inst>
size_t Deafult_Malloc_Template<threads, inst>::_heap_size = 0;	// 总申请内存块的大小

void Test1()
{
	vector<pair<void*, size_t>> v; //顺序表内部成员是一个pair类型的对象
	v.push_back(make_pair(Deafult_Malloc_Template<false, 0>::Allocate(129), 129));//利用make_pair()创建了一个pair类型的对象

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
//	cout << "测试系统堆内存耗尽" << endl;
//
//	SimpleAlloc<char, Alloc>::Allocate(1024 * 1024 * 1024);
//	SimpleAlloc<char, Alloc>::Allocate(1024 * 1024);
//
//	//不好测试，说明系统管理小块内存的能力还是很强的。
//	for (int i = 0; i< 100000; ++i)
//	{
//		char*p1 = SimpleAlloc<char, Alloc>::Allocate(128);
//	}
//
//	cout << "分配完成" << endl;
//}
