#ifdef OS_MMR_WIN
#include <winsock2.h>
//#include <ws2tcpip.h>   // for inet_pton,inet_ntop
#include <windows.h>

#define Thread_ID GetCurrentThreadId()		//获取线程ID
#define Process_ID GetCurrentProcessId()	//获取进程ID
				//函数接口指针
#define STR_OS_TYPE "Windows"	
#else
#include <pthread.h>  
#include <unistd.h>  
#include <dirent.h>
#include <dlfcn.h>
#include <sys/syscall.h>  

#define Thread_ID syscall(SYS_gettid)	//获取线程ID
#define Process_ID getpid()				//获取进程ID

#define STR_OS_TYPE "Linux"

#endif

#include "MemoryPool.hpp"//大块内存分配器
#include "TimeCounter.hpp"

#include <iostream>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <list>
#include <thread>
#include <future>
#include <string>

void TestSmallAllocator();//小内存分配器测试

void TestSmallAlloEffition();//效率测试

void TestChunkAllocator();//大内存分配器

void TestAllocBySize();//测试按大小分配内存

void TestMakeSharedConcurrency();//测试小内存分配器并发


int main()
{
	//TestSmallAllocator();

	//TestSmallAlloEffition();

	//TestChunkAllocator();

	//TestAllocBySize();

	TestMakeSharedConcurrency();

	std::cout << "输入回车继续..." << std::endl;
	std::cin.get();

	return 0;
}

void TestSmallAllocator() 
{
	class AType
	{
	public:
		AType(int lValue)
			:lData(lValue)
		{
			std::cout << "A construct" << std::endl;
		}
		~AType()
		{
			std::cout << "A destruct" << std::endl;
		}
		int lData = 0;
		//char data1[4];//解除屏蔽类大小大于1024
		char data2[1020];
	};


	//测试小内存分配器
	std::cout << "SmallAllocator 分配内存测试 " << std::endl;
	auto allocatorPtr = mmrComm::Singleton<mmrUtil::SmallAllocator>::initInstance();
	auto memInfo = allocatorPtr->getAvailableMemoryInfo();
	std::cout << "memory info " << memInfo.first << "/" << memInfo.second << std::endl;
	{
		auto ptrShare = mmrUtil::Make_Shared<AType>(70);
		auto ptrUnique = mmrUtil::Make_Unique<AType>(700);
		std::cout << "shared ptr AType strValue " << ptrShare->lData << std::endl;
		std::cout << "unique ptr AType strValue " << ptrUnique->lData << std::endl;

		std::shared_ptr<AType> sptr = std::move(ptrUnique);

		memInfo = allocatorPtr->getAvailableMemoryInfo();
		std::cout << "memory info " << memInfo.first << "/" << memInfo.second << std::endl;
	}
	memInfo = allocatorPtr->getAvailableMemoryInfo();
	std::cout << "memory info " << memInfo.first << "/" << memInfo.second << std::endl;
	//allocatorPtr->releaseFreeMemory();
	//memInfo = allocatorPtr->getAvailableMemoryInfo();
	//std::cout << "memory info " << memInfo.first << "/" << memInfo.second << std::endl;
	mmrComm::Singleton<mmrUtil::SmallAllocator>::destroyInstance();
}

void TestSmallAlloEffition()//效率测试
{
	class BType 
	{
	public:
		BType(int data)
			: m_lData(data)
		{

		}
		int m_lData;
		char m_szData[100];
	};

	std::cout << "SmallAllocator 分配内存效率测试 " << std::endl;
	auto allocatorPtr = mmrComm::Singleton<mmrUtil::SmallAllocator>::initInstance();
	size_t count = 1000;
	{
		std::cout << "第一次测试，内存池为空" << std::endl;
		std::vector<std::shared_ptr<BType>> vecAllPtr;
		vecAllPtr.reserve(count);
		mmrUtil::TimeCounter timeCount;
		for (size_t i = 0; i < count; i++)
		{
			vecAllPtr.emplace_back(std::make_shared<BType>(100));
		}
		std::cout << "1 std make shared ptr time cost " << timeCount.elapsed_micro() << " vector size " << vecAllPtr.size() << std::endl;
		vecAllPtr.clear();
		timeCount.reset();
		for (size_t i = 0; i < count; i++)
		{
			vecAllPtr.emplace_back(mmrUtil::Make_Shared<BType>(100));
		}
		std::cout << "1 allocator make ptr time cost " << timeCount.elapsed_micro() << " vector size " << vecAllPtr.size() << std::endl;
		vecAllPtr.clear();
	}

	{
		std::cout << "第二次测试，内存池为非空" << std::endl;
		std::vector<std::shared_ptr<BType>> vecAllPtr;
		vecAllPtr.reserve(count);
		mmrUtil::TimeCounter timeCount;
		for (size_t i = 0; i < count; i++)
		{
			vecAllPtr.emplace_back(std::make_shared<BType>(100));
		}
		std::cout << "2 std make shared ptr time cost " << timeCount.elapsed_micro() << " vector size " << vecAllPtr.size() << std::endl;
		vecAllPtr.clear();
		timeCount.reset();
		for (size_t i = 0; i < count; i++)
		{
			vecAllPtr.emplace_back(mmrUtil::Make_Shared<BType>(100));
		}
		std::cout << "2 allocator make ptr time cost " << timeCount.elapsed_micro() << " vector size " << vecAllPtr.size() << std::endl;
		vecAllPtr.clear();
	}
	auto memInfo = allocatorPtr->getAvailableMemoryInfo();
	std::cout << "memory info before clear " << memInfo.first << "/" << memInfo.second << std::endl;
	//allocatorPtr->releaseFreeMemory();
	//memInfo = allocatorPtr->getAvailableMemoryInfo();
	//std::cout << "memory info after clear " << memInfo.first << "/" << memInfo.second << std::endl;
	mmrComm::Singleton<mmrUtil::SmallAllocator>::destroyInstance();
}

void TestChunkAllocator() 
{
	std::cout << "ChunckAllocator内存分配器测试" << std::endl;
	//初始化内存分配器，参数1：内存过期时间、参数2：最大缓存（MB）
	auto ptrAllocator = mmrComm::Singleton<mmrUtil::ChunkAllocator<>>::initInstance(60, 64);
	auto memInfo = ptrAllocator->getAvailableMemoryInfo();
	std::cout << "内存池中缓存大小(空闲/总数)：" << memInfo.first << "/" << memInfo.second << std::endl;
	{
		auto pariRet = ptrAllocator->allocate<int>(20);//分配第一块内存
		int* oriPtr = pariRet.second.get();
		oriPtr[0] = 5;
		std::cout << "int " << oriPtr[0] << std::endl;
		auto ptrInt2 = ptrAllocator->allocate<int>(20);//分配第二块内存
		auto ptrInt3 = ptrAllocator->allocate<int>(20);//分配第三块内存
		//离开作用域自动归还内存
		memInfo = ptrAllocator->getAvailableMemoryInfo();
		std::cout << "内存池中缓存大小(空闲/总数)：" << memInfo.first << "/" << memInfo.second << std::endl;
	}

	memInfo = ptrAllocator->getAvailableMemoryInfo();
	std::cout << "内存池中缓存大小(空闲/总数)：" << memInfo.first << "/" << memInfo.second << std::endl;

	{
		auto ptrInt2 = ptrAllocator->allocate<int>(512).second;
		int* oriPtr = ptrInt2.get();
		oriPtr[0] = 5;
		std::cout << "int " << oriPtr[0] << std::endl;

		memInfo = ptrAllocator->getAvailableMemoryInfo();
		std::cout << "内存池中缓存大小(空闲/总数)：" << memInfo.first << "/" << memInfo.second << std::endl;
	}

	memInfo = ptrAllocator->getAvailableMemoryInfo();
	std::cout << "内存池中缓存大小：" << memInfo.first << "/" << memInfo.second << std::endl;
	mmrComm::Singleton<mmrUtil::ChunkAllocator<>>::destroyInstance();
}

void TestAllocBySize() 
{
	std::cout << "测试分配指定大小内存" << std::endl;
	//初始化内存分配器，参数1：内存过期时间、参数2：最大缓存（MB）
	auto ptrChunckAllocator = mmrComm::Singleton<mmrUtil::ChunkAllocator<>>::initInstance(60, 64);
	auto ptrSmallAllocator = mmrComm::Singleton<mmrUtil::SmallAllocator>::initInstance();

	auto memSamllInfo = ptrSmallAllocator->getAvailableMemoryInfo();
	auto memChunkInfo = ptrChunckAllocator->getAvailableMemoryInfo();

	{
		uint32_t ulSize = 7;
		auto ptrData1 = mmrUtil::allocateBySize(ulSize);
		memSamllInfo = ptrSmallAllocator->getAvailableMemoryInfo();
		memChunkInfo = ptrChunckAllocator->getAvailableMemoryInfo();
		std::cout << "从小内存分配器分配8字节" << std::endl;
		std::cout << "小内存池中缓存大小：" << memSamllInfo.first << "/" << memSamllInfo.second << std::endl;
		std::cout << "大内存池中缓存大小：" << memChunkInfo.first << "/" << memChunkInfo.second << std::endl;

		ulSize = 1025;
		ptrData1 = mmrUtil::allocateBySize(ulSize);
		memSamllInfo = ptrSmallAllocator->getAvailableMemoryInfo();
		memChunkInfo = ptrChunckAllocator->getAvailableMemoryInfo();
		std::cout << "从大内存分配器分配1025字节" << std::endl;
		std::cout << "小内存池中缓存大小：" << memSamllInfo.first << "/" << memSamllInfo.second << std::endl;
		std::cout << "大内存池中缓存大小：" << memChunkInfo.first << "/" << memChunkInfo.second << std::endl;
	}

	memSamllInfo = ptrSmallAllocator->getAvailableMemoryInfo();
	memChunkInfo = ptrChunckAllocator->getAvailableMemoryInfo();
	std::cout << "全部归还内存" << std::endl;
	std::cout << "小内存池中缓存大小：" << memSamllInfo.first << "/" << memSamllInfo.second << std::endl;
	std::cout << "大内存池中缓存大小：" << memChunkInfo.first << "/" << memChunkInfo.second << std::endl;

	mmrComm::Singleton<mmrUtil::SmallAllocator>::destroyInstance();
	mmrComm::Singleton<mmrUtil::ChunkAllocator<>>::destroyInstance();
}

void TestMakeSharedConcurrency()
{
	struct stData 
	{
		int value[500] = {0};
	};
	//初始化内存分配器，参数1：内存过期时间、参数2：最大缓存（MB）
	auto ptrSmallAllocator = mmrComm::Singleton<mmrUtil::SmallAllocator>::initInstance();

	std::atomic_bool bStart(false);
	uint16_t ckNum = 500000;
	auto funTest = [&](uint16_t num)
	{
		printf("thread %d wait start!\n", Thread_ID);
		while (!bStart);
		printf("thread %d start!\n", Thread_ID);
		{
			mmrUtil::TimeCounter timeCount;
			std::vector<std::shared_ptr<stData>> vecInt32;
			vecInt32.reserve(num);
			for (uint16_t i = 0; i < num; ++i)
			{
				vecInt32.emplace_back(mmrUtil::Make_Shared<stData>());
			}
			printf("thread %d end mmr makeshared! time count %d ms\n", Thread_ID, timeCount.elapsed_milli());
		}

		{
			mmrUtil::TimeCounter timeCount;
			std::vector<std::shared_ptr<stData>> vecInt32;
			vecInt32.reserve(num);
			for (uint16_t i = 0; i < num; ++i)
			{
				vecInt32.emplace_back(std::make_shared<stData>());
			}
			printf("thread %d end std makeshared! time count %d ms\n", Thread_ID, timeCount.elapsed_milli());
		}

		{
			mmrUtil::TimeCounter timeCount;
			std::vector<std::shared_ptr<stData>> vecInt32;
			vecInt32.reserve(num);
			for (uint16_t i = 0; i < num; ++i)
			{
				vecInt32.emplace_back(mmrUtil::Make_Shared<stData>());
			}
			printf("thread %d end mmr makeshared! time count %d ms\n", Thread_ID, timeCount.elapsed_milli());
		}

		{
			mmrUtil::TimeCounter timeCount;
			std::vector<std::shared_ptr<stData>> vecInt32;
			vecInt32.reserve(num);
			for (uint16_t i = 0; i < num; ++i)
			{
				vecInt32.emplace_back(std::make_shared<stData>());
			}
			printf("thread %d end std makeshared! time count %d ms\n", Thread_ID, timeCount.elapsed_milli());
		}
	};

	// 多线程测试，数据量太小，貌似没有真正并发
	auto future1 = std::async(std::launch::async, funTest, ckNum);
	//auto future2 = std::async(std::launch::async, funTest, ckNum);

	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	bStart.store(true);
	future1.get();
	//future2.get();

	auto memSamllInfo = ptrSmallAllocator->getAvailableMemoryInfo();
	std::cout << "小内存池中缓存大小：" << memSamllInfo.first << "/" << memSamllInfo.second << std::endl;
	mmrComm::Singleton<mmrUtil::SmallAllocator>::destroyInstance();
}