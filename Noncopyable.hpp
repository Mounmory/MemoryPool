/**
 * @file Noncopyable.hpp
 * @brief 不可复制类基类
 * @author Mounmory (237628106@qq.com) https://github.com/Mounmory
 * @date 
 *
 * 
 */

#ifndef MMR_COMMON_NOCOPYABLE_H
#define MMR_COMMON_NOCOPYABLE_H

namespace mmrComm 
{

	class NonCopyable
	{
	public:
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable(NonCopyable&&) = delete;
		NonCopyable& operator=(const NonCopyable&) = delete;
		NonCopyable& operator=(NonCopyable&&) = delete;
	protected:
		NonCopyable() = default;
		~NonCopyable() = default;
	};

}
#endif  // COMMON_NOCOPYABLE_H
