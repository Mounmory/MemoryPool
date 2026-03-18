# C++ 内存池实现

## 项目概述

这是一个高效的 C++ 内存池实现，旨在提高内存分配和释放的性能，减少内存碎片，特别是在频繁分配和释放小块内存的场景下。

此项目为 https://github.com/Mounmory/MmrService 中使用的内存池

## 功能特性

- **双分配器设计**：
  - `SmallAllocator`：用于分配小块内存（最大 8192 字节）
  - `ChunkAllocator`：用于分配大块内存
- **智能指针支持**：
  - 提供 `Make_Shared` 和 `Make_Unique` 接口，能够生成标准库的 `std::make_shared` 和 `std::make_unique`对象
- **内存缓存**：
  - 自动缓存已释放的内存，减少系统调用
  - 可配置的缓存过期时间和最大缓存大小
- **线程安全**：
  - 支持多线程并发分配和释放内存
- **内存使用监控**：
  - 提供内存使用情况查询接口
- **内存对齐**：
  - 自动处理内存对齐，提高访问效率

## 代码结构

```
├── ChunkAllocator.cpp      # 大块内存分配器实现
├── ChunkAllocator.h        # 大块内存分配器头文件
├── SmallAllocator.cpp      # 小块内存分配器实现
├── SmallAllocator.h        # 小块内存分配器头文件
├── MemoryPool.hpp          # 内存池接口定义
├── Singleton.hpp           # 单例模式实现
├── Noncopyable.hpp         # 不可复制类实现
├── TimeCounter.hpp         # 时间计数器
├── main.cpp                # 测试示例
├── CMakeLists.txt          # CMake 构建文件
└── README.md               # 项目说明文档
```

## 核心组件

### SmallAllocator

- 用于分配小块内存（最大 8192 字节）
- 内部使用 `FixedAllocator` 管理不同大小的内存块
- 每个 `FixedAllocator` 负责特定大小的内存分配
- 采用无锁设计提高并发性能

### ChunkAllocator

- 用于分配大块内存
- 支持自定义对齐大小（默认 512 字节）
- 内存大小会自动调整为对齐大小的整数倍

### 内存池接口

- `Make_Shared<T>(args...)`：创建共享智能指针对象
- `Make_Unique<T>(args...)`：创建唯一智能指针对象
- `allocateBySize(size)`：根据大小自动选择分配器

## 安装和使用

### 构建项目

使用 CMake 构建项目：

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### 基本使用

#### 1. 构造智能指针对象

```cpp
#include "MemoryPool.hpp"

// 创建共享智能指针
auto ptr = mmrUtil::Make_Shared<MyClass>(constructor_args);

// 创建唯一智能指针
auto uniquePtr = mmrUtil::Make_Unique<MyClass>(constructor_args);
```

#### 2. 分配指定大小的内存

```cpp
#include "MemoryPool.hpp"

// 分配指定大小的内存
uint32_t size = 1024 * 1024; // 1MB
auto memory = mmrUtil::allocateBySize(size);
```

#### 3. 初始化和销毁内存池

```cpp
// 初始化内存池（可选，默认会在首次使用时自动初始化）
auto smallAllocator = mmrComm::Singleton<mmrUtil::SmallAllocator>::initInstance(60, 512); // 60分钟过期，最大缓存512MB
auto chunkAllocator = mmrComm::Singleton<mmrUtil::ChunkAllocator<>>::initInstance(60, 64); // 60分钟过期，最大缓存64MB

// 获取内存使用情况
auto memInfo = smallAllocator->getAvailableMemoryInfo();
std::cout << "内存使用情况: " << memInfo.first << "/" << memInfo.second << std::endl;

// 销毁内存池（可选，程序结束时会自动销毁）
mmrComm::Singleton<mmrUtil::SmallAllocator>::destroyInstance();
mmrComm::Singleton<mmrUtil::ChunkAllocator<>>::destroyInstance();
```

## 性能测试

项目包含多个性能测试示例，位于 `main.cpp` 文件中：

1. `TestSmallAllocator`：测试小块内存分配器的基本功能
2. `TestSmallAlloEffition`：测试小块内存分配器的性能
3. `TestChunkAllocator`：测试大块内存分配器的功能
4. `TestAllocBySize`：测试按大小分配内存的功能
5. `TestMakeSharedConcurrency`：测试并发情况下的性能

## 注意事项

1. **内存大小限制**：
   - `SmallAllocator` 最大支持 8192 字节的对象
   - 超过此大小的对象会自动使用 `ChunkAllocator`

2. **线程安全**：
   - 内存池设计考虑了线程安全，支持多线程并发操作
   - 但在高并发场景下，仍可能存在一定的竞争

3. **内存管理**：
   - 内存池会自动管理内存的分配和释放
   - 可以通过 `releaseFreeMemory()` 手动释放闲置内存

4. **性能优化**：
   - 首次分配内存时，性能可能与标准库相当
   - 内存池预热后，性能会显著提升

## 扩展和定制

1. **调整对齐大小**：
   - 可以通过模板参数调整 `ChunkAllocator` 的对齐大小
   - 例如：`ChunkAllocator<10>` 会使用 1024 字节的对齐大小

2. **调整缓存参数**：
   - 可以在初始化时调整缓存过期时间和最大缓存大小
   - 根据实际应用场景进行优化

3. **添加自定义分配器**：
   - 可以根据需要添加自定义的内存分配器
   - 实现相应的接口即可集成到内存池系统中

## 作者

- **Mounmory**
- GitHub: https://github.com/Mounmory
- Email: 237628106@qq.com
