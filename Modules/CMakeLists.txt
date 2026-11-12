

# Debug 构建为所有模块统一定义 _DEBUG（Release 不定义）。
# 放在 add_subdirectory 之前，使各模块的调试日志共用同一个开关。
add_compile_definitions("$<$<CONFIG:Debug>:_DEBUG>")

add_subdirectory(CommandCore)
add_subdirectory(GwongDongFileSystems)

# 本地可选模块：目录存在时才加入。
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/TS3FuncWrapper")
    add_subdirectory(TS3FuncWrapper)
endif()

add_subdirectory(Loggings)
add_subdirectory(SaiKoLily)
add_subdirectory(Plugin_GwongDong)

