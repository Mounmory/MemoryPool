/**
 * @file ChunkAllocator.h
 * @brief 大块内存分配器类
 * @author Mounmory (237628106@qq.com) https://github.com/Mounmory
 * @date 
 *
 * 
 */

#ifndef MMR_UTIL_ALLOCATOR_H
#define MMR_UTIL_ALLOCATOR_H

#include "Singleton.hpp"
#include "Noncopyable.hpp"


#include <memory>
#include <atomic>
#include <functional>

namespace mmrUtil {


	template<size_t Align = 9>//分配对齐，可分配最小的内存为2^Align
	class ChunkAllocator : public mmrComm::NonCopyable
	{
		static_assert(Align <= 16, "min allocate memory size should larger than 2 ^16 byte.");

		friend class mmrComm::Singleton<mmrUtil::ChunkAllocator<Align>>;

		//缓存清理过期时间（Min），最大缓存数（Mb）
		ChunkAllocator(uint32_t ulExpiredTime = 60, uint32_t ulMaxCache = 64);

	public:
		static constexpr uint32_t _AlignSize = (uint32_t(1) << Align);

		static constexpr uint32_t _LessAlignSize = _AlignSize - 1;

		~ChunkAllocator();

		//获取最大缓存
		uint32_t getMaxCacheSize() const { return m_usMaxFreeCacheSize; }

		//分配指向void指针内存
		template<typename T>
		std::pair<uint32_t, std::shared_ptr<T>> allocate(uint32_t ulElemSize)
		{
			static_assert(std::is_same<std::remove_cv_t<std::remove_reference_t<T>>, T>::value, "type should not be with const or reference!");
			static_assert(std::is_arithmetic<T>::value, "type should be arithmetic!");

			std::pair<uint32_t, std::shared_ptr<T>> pairRet = { 0,nullptr };
			pairRet.first = (ulElemSize * sizeof(T) + _LessAlignSize) & (uint32_t(-1) ^ _LessAlignSize);//最少分配1024Byte内存，且为1024整数倍
			pairRet.second = std::static_pointer_cast<T>(alloSharedMemory(pairRet.first));
			//std::cout << "allocate use count " << pairRet.second.use_count() << std::endl;
			return pairRet;
		}

		//分配指向void指针内存，大小在内部修正
		std::unique_ptr<void, std::function<void(void*)>> allocateBySize(uint32_t& ulDataSize);

		//释放过期内存
		void freeExpiredMemory();

		//处理定时器
		void onTimer();

		//处理命令行控制命令
		void loop(std::atomic_bool& bRunFlag);

		//获取内存使用情况<free/total>
		std::pair<size_t, size_t> getAvailableMemoryInfo() const { return { m_sizeFree.load(std::memory_order_relaxed),m_sizeTotal.load(std::memory_order_relaxed) }; }
	private:
		//分配指向void共享智能指针内存
		std::shared_ptr<void> alloSharedMemory(uint32_t ulElemSize);

		//分配原始指针内存
		void* alloRowMemory(uint32_t ulElemSize);

		//归还内存
		void deallocate(uint32_t ulBufSize, void* pBuf);

	private:
		const uint32_t m_ulExpireTime;//过期时间，单位min,默认60min
		const uint32_t m_usMaxFreeCacheSize;//最大内存大小,默认4M

		std::atomic<uint32_t> m_sizeFree;//空闲内存，单位byte
		std::atomic<uint32_t> m_sizeTotal;//总的内存，单位byte
	private:
		struct DataImp;
		std::unique_ptr<DataImp> m_ptrData;
	};
	template class mmrUtil::ChunkAllocator<>;
	//template class mmrUtil::ChunkAllocator<9>;
	//template class mmrUtil::ChunkAllocator<8>;

	template class mmrComm::Singleton<mmrUtil::ChunkAllocator<>>;//默认最小分配内存
	//template class mmrComm::Singleton<mmrUtil::ChunkAllocator<9>>;
	//template class mmrComm::Singleton<mmrUtil::ChunkAllocator<8>>;
}


#endif
