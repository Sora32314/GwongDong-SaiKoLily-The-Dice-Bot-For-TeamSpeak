set(lib_name Plugin_GwongDong)
set(command_core_dir ${CMAKE_SOURCE_DIR}/Modules/CommandCore)
set(saiko_lily_dir ${CMAKE_SOURCE_DIR}/Modules/SaiKoLily)
set(encapsulation_spdlog_dir ${CMAKE_SOURCE_DIR}/Modules/Loggings)
set(teamspeak_sdk_dir ${CMAKE_SOURCE_DIR}/ExternModules/spdlog-1.15.3/ts3client-pluginsdk-26/include)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src)
    message(STATUS "生成中！")
    file(GLOB_RECURSE SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    )
endif()

add_library("${lib_name}" SHARED ${SOURCES})


target_include_directories(${lib_name}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${teamspeak_sdk_dir}
    ${encapsulation_spdlog_dir}/include
    ${saiko_lily_dir}/include
    ${command_core_dir}/include
)

if(MSVC)
    target_compile_definitions(${lib_name} PRIVATE "LIB_EXPORT=__declspec(dllexport)")
    #target_compile_definitions(other_lib PRIVATE "LIB_IMPORT=__declspec(dllimport)")
endif()


target_link_libraries(${lib_name} PRIVATE command_core)
target_link_libraries(${lib_name} PRIVATE encapsulation_spdlog)
target_link_libraries(${lib_name} PRIVATE saiko_lily)

