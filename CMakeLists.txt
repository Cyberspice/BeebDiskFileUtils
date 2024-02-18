cmake_minimum_required(VERSION 3.10)

set(DFSUTILS_SOURCES src/dfsutil.c src/diskimagefile.c src/dfs.c src/debug.c src/acornfs.c)

project(dfsutils)
add_executable(dfsutils ${DFSUTILS_SOURCES})
target_include_directories(dfsutils PRIVATE include)
