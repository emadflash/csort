cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

set(CMAKE_C_STANDARD 99)
set(PROJECT_NAME "csort")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_C_FLAGS_DEBUG "-g -fsanitize=address -O0")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

project(${PROJECT_NAME} DESCRIPTION "import sorter for python")
option(BUILD_TEST "Build Test" OFF)
option(BUILD_RELEASE "Build Release" ON)

# Compile external/lua
set (LUA_DIR "./external/lua")
add_library(lualib STATIC
    ${LUA_DIR}/lapi.h
    ${LUA_DIR}/lauxlib.h
    ${LUA_DIR}/lcode.h
    ${LUA_DIR}/lctype.h
    ${LUA_DIR}/ldebug.h
    ${LUA_DIR}/ldo.h
    ${LUA_DIR}/lfunc.h
    ${LUA_DIR}/lgc.h
    ${LUA_DIR}/ljumptab.h
    ${LUA_DIR}/llex.h
    ${LUA_DIR}/llimits.h
    ${LUA_DIR}/lmem.h
    ${LUA_DIR}/lobject.h
    ${LUA_DIR}/lopcodes.h
    ${LUA_DIR}/lopnames.h
    ${LUA_DIR}/lparser.h
    ${LUA_DIR}/lprefix.h
    ${LUA_DIR}/lstate.h
    ${LUA_DIR}/lstring.h
    ${LUA_DIR}/ltable.h
    ${LUA_DIR}/ltests.h
    ${LUA_DIR}/ltm.h
    ${LUA_DIR}/lua.h
    ${LUA_DIR}/luaconf.h
    ${LUA_DIR}/lualib.h
    ${LUA_DIR}/lundump.h
    ${LUA_DIR}/lvm.h
    ${LUA_DIR}/lzio.h
    ${LUA_DIR}/lapi.c
    ${LUA_DIR}/lauxlib.c
    ${LUA_DIR}/lbaselib.c
    ${LUA_DIR}/lcode.c
    ${LUA_DIR}/lcorolib.c
    ${LUA_DIR}/lctype.c
    ${LUA_DIR}/ldblib.c
    ${LUA_DIR}/ldebug.c
    ${LUA_DIR}/ldo.c
    ${LUA_DIR}/ldump.c
    ${LUA_DIR}/lfunc.c
    ${LUA_DIR}/lgc.c
    ${LUA_DIR}/linit.c
    ${LUA_DIR}/liolib.c
    ${LUA_DIR}/llex.c
    ${LUA_DIR}/lmathlib.c
    ${LUA_DIR}/lmem.c
    ${LUA_DIR}/loadlib.c
    ${LUA_DIR}/lobject.c
    ${LUA_DIR}/lopcodes.c
    ${LUA_DIR}/loslib.c
    ${LUA_DIR}/lparser.c
    ${LUA_DIR}/lstate.c
    ${LUA_DIR}/lstring.c
    ${LUA_DIR}/lstrlib.c
    ${LUA_DIR}/ltable.c
    ${LUA_DIR}/ltablib.c
    ${LUA_DIR}/ltests.c
    ${LUA_DIR}/ltm.c
    ${LUA_DIR}/lua.c
    ${LUA_DIR}/lundump.c
    ${LUA_DIR}/lutf8lib.c
    ${LUA_DIR}/lvm.c
    ${LUA_DIR}/lzio.c
    ${LUA_DIR}/onelua.c
)

add_library(core SHARED
    core.h
    core.c
)

add_library(csortlib SHARED
    csort.h
    csort.c
)
target_link_libraries(csortlib
    core
)

if(BUILD_TEST)
    add_executable(test
        test/test.h
        test/test.c
        config.h
        config.c
    )
    target_link_libraries(test core lualib m csortlib)
endif()

if(BUILD_RELEASE)
add_executable(${PROJECT_NAME}
    main.c
    config.h
    config.c
)
target_link_libraries(${PROJECT_NAME} 
    lualib 
    m
    core
    csortlib
)
endif()
