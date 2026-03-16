set(lib_name encapsulation_spdlog)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)


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
        ${PROJECT_SOURCE_DIR}/ExternModules/spdlog-1.15.3/include
)


# if(MSVC)
#     target_compile_definitions(${lib_name} PRIVATE "LIB_EXPORT=__declspec(dllexport)")
#     #target_compile_definitions(other_lib PRIVATE "LIB_IMPORT=__declspec(dllimport)")
# endif()

target_link_libraries(${lib_name} 
    PUBLIC spdlog::spdlog 
    $<$<BOOL:${MINGW}>:ws2_32>
)

