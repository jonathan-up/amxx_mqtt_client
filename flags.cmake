# Extra compile flags for GCC (AmxxEasyHttp-style). Root CMakeLists already applies -m32 and static libgcc/libstdc++.
set(COMPILE_FLAGS "-U_FORTIFY_SOURCE")

set(COMPILE_FLAGS "${COMPILE_FLAGS} -Wall -fno-builtin -Wno-unknown-pragmas")
set(COMPILE_FLAGS "${COMPILE_FLAGS} -ffunction-sections -fdata-sections")
set(COMPILE_FLAGS "${COMPILE_FLAGS} -mtune=generic -msse3 -fno-sized-deallocation -Wno-strict-aliasing")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 8.0)
    set(COMPILE_FLAGS "${COMPILE_FLAGS} -fcf-protection=none")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(COMPILE_FLAGS "${COMPILE_FLAGS} -g3 -O3 -ggdb")
else()
    set(COMPILE_FLAGS "${COMPILE_FLAGS} -g0 -O3 -fno-stack-protector")
endif()
