cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(mmrMemoryPool)

if(WIN32)
	message("platform windows")
	if(MSVC)
		message("MSVC version ${MSVC_VERSION}")

		#STRING(REPLACE "-O2" "-Od" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})#取消优化
		if(MSVC_VERSION LESS 1900) # 判断编译器版本是否低于VS2015
			message(FATAL_ERROR "MSVC version must be at least 14.0 (Visual Studio 2015)!")
		endif()

		#设置C++编译选项
		if(MSVC_VERSION GREATER_EQUAL 1916)#vs 2017 15.7开始完全支持c++17
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
			set(CPP_17_FLAG ON)
		else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++14")
		endif()
		#add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
		#add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj  /Zc:__cplusplus /source-charset:utf-8")#源文件编码方式
		#设置C编译选项
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /source-charset:utf-8")

		#add_compile_options(/wd4828)#取消4828编译警告
		#option(ENABLE_WINDUMP "Windows MiniDumpWriteDump" "OFF")
		add_definitions(-DOS_MMR_WIN)#添加window系统宏

		#关闭一些VS警告
		add_definitions(
		/wd4065 # switch statement contains 'default' but no 'case' labels
		/wd4244 # 'conversion' conversion from 'type1' to 'type2', possible loss of data
		/wd4251 # 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
		/wd4267 # 'var' : conversion from 'size_t' to 'type', possible loss of data
		/wd4305 # 'identifier' : truncation from 'type1' to 'type2'
		/wd4307 # 'operator' : integral constant overflow
		/wd4309 # 'conversion' : truncation of constant value
		/wd4334 # 'operator' : result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
		/wd4355 # 'this' : used in base member initializer list
		/wd4506 # no definition for inline function 'function'
		/wd4800 # 'type' : forcing value to bool 'true' or 'false' (performance warning)
		/wd4996 # The compiler encountered a deprecated declaration.
		)

	else()
		message(FATAL_ERROR "only msvc project can be generated!")
	endif()
elseif(UNIX)
	message("platform linux")
	#设置C++编译选项
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -fPIC  --coverage")
	#设置c++版本
	if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 10.0)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
		set(CPP_20_FLAG ON)
		set(CPP_17_FLAG ON)
	elseif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 8.0)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
		set(CPP_17_FLAG ON)
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
	endif()
	#message("CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}")
	#message("CMAKE_CXX_COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION}")
	#设置C编译选项
	#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
	
	#link_directories(${THIRD_PART_DIR}/lib/linux)#提前安装到系统
	#link_directories(${THIRD_PART_DIR}/bin/linux)
	add_definitions(-DOS_MMR_LINUX)#添加linux系统宏
else()	
	message(FATAL_ERROR "Unsupported OS type!")
endif()

# 自动查找当前目录下所有的.cpp、.h和.hpp文件
file(GLOB SOURCE_FILES "*.h" "*.hpp" "*.cpp")

# 创建可执行文件
add_executable(MemoryPool ${SOURCE_FILES})

if(NOT WIN32)
	target_link_libraries(MemoryPool pthread)
endif()