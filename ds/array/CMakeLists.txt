cmake_minimum_required(VERSION 3.16)
project(ThreadSafeArray CXX)

# 设置C++标准为C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项，开启警告和优化
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    # 添加优化选项
    add_compile_options(-O2)
    # 如果需要调试，可以使用以下选项
    # add_compile_options(-g -O0)
elseif(MSVC)
    add_compile_options(/W4 /WX)
endif()

# 包含头文件目录
include_directories(include)

# 收集源文件
file(GLOB SOURCES "src/*.cpp")

# 创建可执行文件
add_executable(thread_safe_array_demo ${SOURCES})

# 链接线程库（对于使用std::thread的程序可能需要）
find_package(Threads REQUIRED)
target_link_libraries(thread_safe_array_demo Threads::Threads)

# 如果使用共享库方式构建，可以取消下面的注释
# add_library(thread_safe_array SHARED src/thread_safe_array.cpp)
# target_include_directories(thread_safe_array PUBLIC include)
# target_link_libraries(thread_safe_array_demo thread_safe_array)

# 安装配置（可选）
# install(TARGETS thread_safe_array_demo DESTINATION bin)
# install(FILES include/thread_safe_array.h DESTINATION include/ds)

# 单元测试（可选）
# enable_testing()
# add_subdirectory(tests)