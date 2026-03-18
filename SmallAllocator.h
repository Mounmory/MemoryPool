/**
 * @file SamllAllocator.h
 * @brief 内存分配器,用于构造智能指针对象
 * @author Mounmory (237628106@qq.com) https://github.com/Mounmory
 * @date 
 *
 * 
 */

#ifndef MMR_UTIL_SMALL_ALLOCATOR_H
#define MMR_UTIL_SMALL_ALLOCATOR_H
#include "Singleton.hpp"
#include "Noncopyable.hpp"

#include <type_traits>
#include <memory>
#include <atomic>
#include <vector>
#include <list>
#include <mutex>
#include <unordered_map>
#include <functional>
#include <cassert> 


#define SMALL_OBJECT_MAX_SIZE		8192 //SmallAllocator能分配的最大内存

//#define TINY_OBJECT_MAX_SIZE		1024 //更小的内存与小内存的分界线

namespace mmrUtil
{

	/*
		固定内存分配器，用于分配指定大小的内存
	*/
	class SmallAllocator;

	class FixedAllocator : public mmrComm::NonCopyable
	{
		class ChunkLockFree : public mmrComm::NonCopyable //无锁的内存分配块
		{
			/*
				一大块内存，用于分配小块内存，每个实例可分配指定大小的内存，在init时设置参数
				每个块可分配最多255块内存
			*/
		public:
			ChunkLockFree();
			~ChunkLockFree();

			//要分配的内存块大小，块数量
			void init(std::size_t blockSize, uint8_t blockNum);

			//分配内存
			void* allocate();

			//归还内存
			void deallocate(void* ptr);

			//释放内存，请确保执行前所有分配内存已归还，否则会抛出异常
			void release();

			//可用的内存块数量
			const uint8_t availadeBlockNum() const { return m_availaBlockNum.load(std::memory_order_relaxed); }

			//总内存块数量
			const uint8_t blockNum() const { return m_blockNum; }
		private:
			uint8_t* m_pData;//大块内存首地址
			size_t m_blockSize; //每次分配的内存大小
			std::atomic<uint8_t> m_availaBlockCurr;//当前可用内存索引
			std::atomic<uint8_t> m_availaBlockNum;//剩余可用内存块数量
			uint8_t m_blockNum;//结束点指向索引
		};

		using ChunkType = ChunkLockFree;
	public:
		explicit FixedAllocator(std::size_t blockSize);//blockSize：负责分配的内存的大小

		~FixedAllocator() = default;

		//分配内存并构造共享智能指针对象
		template<typename Type, typename... Args>
		std::shared_ptr<Type> Make_Shared(Args&&... args)
		{
			assert(sizeof(Type) <= m_blockSize);
			auto pairData = allocate();
			new (pairData.first)Type(std::forward<Args>(args)...);//调用构造函数
			return std::shared_ptr<Type>(reinterpret_cast<Type*>(pairData.first),
				[=](Type* ptr)
			{	ptr->~Type();//调用析构
			pairData.second->deallocate(ptr);
			});
		}

		//分配内存并构造独享智能指针对象
		template<typename Type, typename... Args>
		std::unique_ptr<Type, std::function<void(Type*)>> Make_Unique(Args&&... args)
		{
			assert(sizeof(Type) <= m_blockSize);
			auto pairData = allocate();
			new (pairData.first)Type(std::forward<Args>(args)...);//调用构造函数
			return std::unique_ptr<Type, std::function<void(Type*)>>(reinterpret_cast<Type*>(pairData.first),
				[=](Type* ptr)
			{
				ptr->~Type();//调用析构
				pairData.second->deallocate(ptr);
			});
		}

		std::unique_ptr<void, std::function<void(void*)>> allocateUnique();

		//FixAllocator总大小
		std::size_t getTotleMemorySize() const { return m_listChunks.size() * m_blockSize * m_numBlocks; }

		//获取可用内存大小
		std::size_t getAvailableMemorySize();

		//释放闲置内存块
		void releaseFreeMemory();

	private:
		//使用内部内存块分配内存，返回<分配的内存地址，属于哪个chunk指针>
		std::pair<void*, ChunkType*> allocate();

	private:
		const std::size_t m_blockSize;//用于每次分配m_blockSize大小的内存
		uint8_t m_numBlocks;//每个chunk中的内存数量
		ChunkType* m_ptrAllocChunk;//当前正在分配内存的chunk

		std::mutex m_mutexChunks;
		std::list<std::unique_ptr<ChunkType>> m_listChunks;//所有的chunk内存块
	};


	class SmallAllocator : public mmrComm::NonCopyable
	{
		friend class mmrComm::Singleton<mmrUtil::SmallAllocator>;

		//缓存清理过期时间（Min），最大缓存数（Mb）
		SmallAllocator(uint32_t ulExpiredTime = 60, uint32_t ulMaxCache = 512);

	public:
		static constexpr size_t _SmallObjMaxSize = SMALL_OBJECT_MAX_SIZE;//所能小对象上线
		static constexpr size_t _SmallObjStepSize = sizeof(void*);//内存池分配步长，考虑为对象分配的内存地址对齐
		static constexpr size_t _LessSmallObjStepSize = _SmallObjStepSize - 1;//内存池分配步长，考虑为对象分配的内存地址对齐

		~SmallAllocator();

		//分配指向void指针内存，只分配小于_SmallObjMaxSize大小的数据，否则返回空指针
		std::unique_ptr<void, std::function<void(void*)>> allocateBySize(uint32_t& ulDataSize);

		//处理定时器
		void onTimer();

		//处理命令行控制命令
		void loop(std::atomic_bool& bRunFlag);

		//获取内存使用情况<free/total>
		std::pair<size_t, size_t> getAvailableMemoryInfo();

		//根据尺寸获取内存分配器
		FixedAllocator* getFixedAllocator(size_t ulDataSize);
	private:
		void releaseFreeMemory();

	private:
		const uint32_t m_ulExpireTime;//过期时间，单位min,默认60min
		const uint32_t m_usMaxFreeCacheSize;//最大空闲内存大小,默认4M

		//对于大于1024KB的对象分配内存用
		std::mutex m_mutexMapPool;//大对象内存分配互斥锁
		std::unordered_map<size_t, std::unique_ptr<FixedAllocator>> m_mapPool;
	};

	//导出SmallAllocator模板实例
	template class mmrComm::Singleton<mmrUtil::SmallAllocator>;

}

#endif
