#构建最小CMake版本:4.0.1
cmake_minimum_required(VERSION 4.0.1)

#外部库路径
set(exlib_loggings_dir ${CMAKE_SOURCE_DIR}/ExternModules/spdlog-1.15.3/)

#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

#项目名称，使用语言C++
project(TSRobot-GwongDong VERSION 0.0.1 LANGUAGES CXX)





set(CMAKE_CXX_SCAN_FOR_MODULES ON)


add_subdirectory("./Modules")
add_subdirectory(${exlib_loggings_dir})
#add_subdirectory("./MainProject")
