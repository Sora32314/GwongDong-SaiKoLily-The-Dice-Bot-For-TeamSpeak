option(ENABLE_CPPCHECK "Enable cppcheck analysis targets" OFF)

if(NOT ENABLE_CPPCHECK)
    return()
endif()

find_program(CPPCHECK_EXE
    NAMES cppcheck
    HINTS "C:/Program Files/Cppcheck" "$ENV{PROGRAMFILES}/Cppcheck"
)
if(NOT CPPCHECK_EXE)
    message(WARNING "cppcheck not found, analysis targets disabled!")
    return()
endif()

set(CPPCHECK_ARGS
    "--project=${CMAKE_BINARY_DIR}/compile_commands.json"
    --platform=win64
    --library=windows
    --std=c++23
    "--enable=warning,performance,portability"
    --inline-suppr
    --suppress=missingIncludeSystem
    --suppress=missingInclude
    --suppress=unusedFunction
    "--suppress=*:*ExternModules*"               # 关键：屏蔽第三方头文件
    --error-exitcode=2                           # CI 用；本地只看报告就删掉这行
    -i "${CMAKE_BINARY_DIR}"
    -i "${CMAKE_SOURCE_DIR}/ExternModules"
)

add_custom_target(cppcheck
  COMMAND ${CPPCHECK_EXE} ${CPPCHECK_ARGS}
          "--template={file}:{line}:{column}: {severity}: {message} [{id}]"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  USES_TERMINAL VERBATIM
  COMMENT "Running cppcheck on the whole project"
)

add_custom_target(cppcheck-xml
  COMMAND ${CPPCHECK_EXE} ${CPPCHECK_ARGS} --xml --xml-version=2
          "--output-file=${CMAKE_BINARY_DIR}/cppcheck.xml"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  USES_TERMINAL VERBATIM
  COMMENT "Generating cppcheck XML report"
)