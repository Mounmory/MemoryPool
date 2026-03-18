/**
 * @file MemoryPool.hpp
 * @brief 定义大块内存和小块内存分配器使用接口
 * @author Mounmory (237628106@qq.com) https://github.com/Mounmory
 * @date 
 *
 * 
 */
 
#ifndef MMR_UTIL_MEMORY_POOL_HPP
#define MMR_UTIL_MEMORY_POOL_HPP
#include "ChunkAllocator.h"
#include "SmallAllocator.h"
 
namespace mmrUtil {

	template<typename Type, typename... Args>
	std::shared_ptr<Type> Make_Shared(Args&&... args)
	{
		static_assert(sizeof(Type) <= SMALL_OBJECT_MAX_SIZE, "object size can not be dealed.");//如果你想构造很大的对象，也可以取消这个静态断言
		static FixedAllocator* ptrAlloc = mmrComm::Singleton<mmrUtil::SmallAllocator>::getInstance()->getFixedAllocator(sizeof(Type));
		return ptrAlloc->Make_Shared<Type>(std::forward<Args>(args)...);
	}

	template<typename Type, typename... Args>
	std::unique_ptr<Type, std::function<void(Type*)>> Make_Unique(Args&&... args)
	{
		static_assert(sizeof(Type) <= SMALL_OBJECT_MAX_SIZE, "object size can not be deal.");//如果你想构造很大的对象，也可以取消这个静态断言
		static FixedAllocator* ptrAlloc = mmrComm::Singleton<mmrUtil::SmallAllocator>::getInstance()->getFixedAllocator(sizeof(Type));
		return ptrAlloc->Make_Unique<Type>(std::forward<Args>(args)...);
	}

	std::unique_ptr<void, std::function<void(void*)>> allocateBySize(uint32_t& ulDataSize) 
	{
		if (ulDataSize <= SmallAllocator::_SmallObjMaxSize)
		{
			static auto allInstance = mmrComm::Singleton<mmrUtil::SmallAllocator>::getInstance();
			return allInstance->allocateBySize(ulDataSize);
		}
		else 
		{
			static auto allInstance = mmrComm::Singleton<mmrUtil::ChunkAllocator<>>::getInstance();
			return allInstance->allocateBySize(ulDataSize);
		}
	}
}

 #endif //MMR_UTIL_MEMORY_POOL_HPP