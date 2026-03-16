set(lib_name command_core)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(encapsulation_spdlog_dir ${CMAKE_SOURCE_DIR}/Modules/Loggings)


if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src)
    message(STATUS "生成中！")
    file(GLOB_RECURSE SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    )
endif()

add_library(${lib_name} STATIC ${SOURCES})

set_target_properties(${lib_name} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    C_VISIBILITY_PRESET hidden     # C 代码
    CXX_VISIBILITY_PRESET hidden   # C++ 代码
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(${lib_name}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${encapsulation_spdlog_dir}/include
)

target_link_libraries(${lib_name} PRIVATE encapsulation_spdlog)