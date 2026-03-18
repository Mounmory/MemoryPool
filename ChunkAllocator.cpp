#include "ChunkAllocator.h"

#include <mutex>
#include <unordered_map>
#include <map>
#include <deque>
#include <chrono>
#include <iostream>

using namespace mmrUtil;

using TimeType = std::chrono::system_clock::time_point;

template<size_t Align>
struct ChunkAllocator<Align>::DataImp
{
	std::map<uint32_t, std::deque<std::pair<TimeType, void*>> > memBuffer;
	std::mutex mt;

	void clear() 
	{
		for (auto& p : memBuffer)
		{
			auto& refVec = p.second;
			for (auto& p1 : refVec)
			{
				char* buf = (char*)(p1.second);
				delete[]buf;
			}
			refVec.clear();
		}
	}
};

template<size_t Align>
ChunkAllocator<Align>::ChunkAllocator(uint32_t ulExpiredTime, uint32_t ulMaxCache)
	: m_ulExpireTime(ulExpiredTime)
	, m_usMaxFreeCacheSize(ulMaxCache * 1024 * 1024)//4M
	, m_sizeFree(0)
	, m_sizeTotal(0)
	, m_ptrData(std::make_unique<DataImp>())
{
	std::cout << "ChunkAllocator instance construct: expired time[" << m_ulExpireTime << " Min] Max Cache size [" << ulMaxCache << " Mb]" << std::endl;
}

template<size_t Align>
ChunkAllocator<Align>::~ChunkAllocator()
{
	m_ptrData->clear();
	m_sizeFree.store(0, std::memory_order_relaxed);
	m_sizeTotal.store(0, std::memory_order_relaxed);
	std::cout << "ChunkAllocator instance destruct." << std::endl;
}

template<size_t Align>
std::unique_ptr<void, std::function<void(void*)>>  ChunkAllocator<Align>::allocateBySize(uint32_t& ulDataSize)
{
	ulDataSize = (ulDataSize + _LessAlignSize) & (uint32_t(-1) ^ _LessAlignSize);//最少分配1024Byte内存，且为1024整数倍
	void* ptrVoid = alloRowMemory(ulDataSize);//将锁操作封装独立接口，避免new内存时延长锁时长
	if (nullptr == ptrVoid)
	{
		ptrVoid = new char[ulDataSize];
		m_sizeTotal.fetch_add(ulDataSize, std::memory_order_relaxed);
	}
	return std::unique_ptr<void, std::function<void(void*)>>(ptrVoid, [=](void* pBuf) {deallocate(ulDataSize, pBuf); });
}

template<size_t Align>
void ChunkAllocator<Align>::freeExpiredMemory() 
{
	//清理过期的缓存
	auto timeNow = std::chrono::system_clock::now();
	std::lock_guard<std::mutex> guard(m_ptrData->mt);
	for (auto iterMemQueue = m_ptrData->memBuffer.begin(); iterMemQueue != m_ptrData->memBuffer.end();)
	{
		auto& queMem = iterMemQueue->second;
		for (auto iterBlock = queMem.begin(); iterBlock != queMem.end(); )
		{
			if (std::chrono::duration_cast<std::chrono::minutes>(timeNow - iterBlock->first).count() > m_ulExpireTime)
			{
				char* buf = (char*)(iterBlock->second);
				delete[]buf;
				//空闲内存减少
				m_sizeFree.fetch_sub(iterMemQueue->first, std::memory_order_relaxed);
				//总内存减少
				m_sizeTotal.fetch_sub(iterMemQueue->first, std::memory_order_relaxed);
				iterBlock = iterMemQueue->second.erase(iterBlock);
			}
			else
			{
				++iterBlock;
			}
		}
		if (queMem.size() == 0)
		{
			iterMemQueue = m_ptrData->memBuffer.erase(iterMemQueue);
		}
		else
		{
			++iterMemQueue;
		}
	}
}

template<size_t Align>
void ChunkAllocator<Align>::onTimer()
{
	freeExpiredMemory();
	auto pairMemInfo = getAvailableMemoryInfo();
	printf("memory info after clear is (free/total) %lld/%lld Byte"
		, pairMemInfo.first, pairMemInfo.second);
}

template<size_t Align>
void ChunkAllocator<Align>::loop(std::atomic_bool& bRunFlag)
{
	//auto pairAllocMemInfo = getAvailableMemoryInfo();
	//printf("Chunk allocator memory pool info:\n"
	//	"\tChunk allocator memory(free/total): %lld/%lld Byte (%s/%s)\n"
	//	, pairAllocMemInfo.first
	//	, pairAllocMemInfo.second
	//	, formatMemorySize(pairAllocMemInfo.first).c_str()
	//	, formatMemorySize(pairAllocMemInfo.second).c_str());
}

template<size_t Align>
std::shared_ptr<void> ChunkAllocator<Align>::alloSharedMemory(uint32_t ulElemSize)
{
	void* ptrVoid = alloRowMemory(ulElemSize);//将锁操作封装独立接口，避免new内存时延长锁时长
	if (nullptr == ptrVoid) 
	{
		ptrVoid = new char[ulElemSize];
		m_sizeTotal.fetch_add(ulElemSize, std::memory_order_relaxed);
	}
	return std::shared_ptr<void>(ptrVoid, [=](void* pBuf) {deallocate(ulElemSize, pBuf); });
}

template<size_t Align>
void * ChunkAllocator<Align>::alloRowMemory(uint32_t ulElemSize)
{
	void* ptrVoid = nullptr;
	std::lock_guard<std::mutex> guard(m_ptrData->mt);
	auto iterBuf = m_ptrData->memBuffer.find(ulElemSize);
	if (iterBuf != m_ptrData->memBuffer.end())
	{
		ptrVoid = iterBuf->second.back().second;
		iterBuf->second.pop_back();
		m_sizeFree.fetch_sub(ulElemSize, std::memory_order_relaxed);
	}
	return ptrVoid;
}

template<size_t Align>
void ChunkAllocator<Align>::deallocate(uint32_t ulBufSize, void* pBuf)
{
	m_sizeFree.fetch_add(ulBufSize, std::memory_order_relaxed);
	std::lock_guard<std::mutex> guard(m_ptrData->mt);
	m_ptrData->memBuffer[ulBufSize].push_back(std::make_pair(std::chrono::system_clock::now(), pBuf));
	//std::cout << "deallocate size " << ulBufSize << std::endl;
	if (m_sizeFree.load(std::memory_order_relaxed) > m_usMaxFreeCacheSize)
	{
		//if (logInstancePtr) 
		//{
		//	printf("Before clear cache, free cache size[%ld],MaxFreeCacheSize[%ld].", 
		//		m_sizeFree.load(std::memory_order_relaxed), m_usMaxFreeCacheSize);
		//}
		//清理过期的缓存
		auto timeNow = std::chrono::system_clock::now();
		for (auto iterMemQueue = m_ptrData->memBuffer.begin(); iterMemQueue != m_ptrData->memBuffer.end();)
		{
			auto& queMem = iterMemQueue->second;
			for (auto iterBlock = queMem.begin();iterBlock != queMem.end(); )
			{
				if (std::chrono::duration_cast<std::chrono::minutes>(timeNow - iterBlock->first).count() > m_ulExpireTime) 
				{
					char* buf = (char*)(iterBlock->second);
					delete[]buf;

					m_sizeFree.fetch_sub(iterMemQueue->first, std::memory_order_relaxed);
					m_sizeTotal.fetch_sub(iterMemQueue->first, std::memory_order_relaxed);
					iterBlock = iterMemQueue->second.erase(iterBlock);
				}
				else 
				{
					++iterBlock;
				}
			}
			if (queMem.size() == 0)
			{
				iterMemQueue = m_ptrData->memBuffer.erase(iterMemQueue);
			}
			else 
			{
				++iterMemQueue;
			}
		}
		//如果仍然超过最大值，把内存最大的一块清理，肯定小于最大限了
		if (m_sizeFree.load(std::memory_order_relaxed) > m_usMaxFreeCacheSize)
		{
			auto iterRMem = m_ptrData->memBuffer.rbegin();
			if (iterRMem != m_ptrData->memBuffer.rend() && iterRMem->second.size() > 0)//肯定是成立的
			{
				auto& pairBack = iterRMem->second.front();
				char* buf = (char*)(pairBack.second);
				delete[]buf;
				iterRMem->second.pop_front();
				m_sizeFree.fetch_sub(iterRMem->first, std::memory_order_relaxed);
				m_sizeTotal.fetch_sub(iterRMem->first, std::memory_order_relaxed);
				if (iterRMem->second.size() == 0)
				{
					m_ptrData->memBuffer.erase(std::next(iterRMem).base());
				}
			}
		}
		//if (logInstancePtr)
		//{
		//	printf("After clear cache, free cache size[%ld], total cache size [%ld].", 
		//		m_sizeFree.load(std::memory_order_relaxed), m_sizeTotal.load(std::memory_order_relaxed));
		//}
	}
}


