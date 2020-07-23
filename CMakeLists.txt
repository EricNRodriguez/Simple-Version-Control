cmake_minimum_required(VERSION 3.15)
project(assignment-2-svc C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS_DEBUG "-D_DEBUG")

add_library(assignment-2-svc svc.c svc.h memory/safe_alloc.c memory/string_copy.c memory/memory.h branch/branch.c branch/branch.h  commit/commit.c commit/commit.h snapshot/snapshot.c snapshot/snapshot.h )
