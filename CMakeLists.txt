#构建最小CMake版本:3.13.0
cmake_minimum_required(VERSION 3.31.0)

#外部库路径
set(exlib_loggings_dir ${CMAKE_SOURCE_DIR}/ExternModules/spdlog-1.15.3/)

#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")


#项目名称，使用语言C++
project(TSRobot-GwongDong VERSION 0.0.1 LANGUAGES CXX)


if(UNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    #防止去除GLIBCXX依赖
    add_link_options(-static-libstdc++)
    add_link_options(-static-libgcc)
endif()


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(Cppcheck)


set(CMAKE_CXX_SCAN_FOR_MODULES ON)

#离线逻辑试验台（TestSpace）：默认开启；不需要时可 -DGD_BUILD_TESTSPACE=OFF 关闭
option(GD_BUILD_TESTSPACE "Build offline logic test benches under TestSpace/" ON)


add_subdirectory("./Modules")
add_subdirectory(${exlib_loggings_dir})

#试验台依赖 Modules 下创建的库目标，必须放在其后
if(GD_BUILD_TESTSPACE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/TestSpace")
    add_subdirectory("./TestSpace")
endif()

#add_subdirectory("./MainProject")
