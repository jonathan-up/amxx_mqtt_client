SET(COMPILE_FLAGS "-m32 -U_FORTIFY_SOURCE")
SET(LINK_FLAGS "-m32 -s -static-libstdc++ -static-libgcc")

# 构建编译参数
SET(COMPILE_FLAGS "${COMPILE_FLAGS} -Wall -fno-builtin -Wno-unknown-pragmas")
SET(COMPILE_FLAGS "${COMPILE_FLAGS} -ffunction-sections -fdata-sections")
SET(COMPILE_FLAGS "${COMPILE_FLAGS} -mtune=generic -msse3 -fno-sized-deallocation -Wno-strict-aliasing")

# GCC >= 8.3
IF (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 8.0)
    SET(COMPILE_FLAGS "${COMPILE_FLAGS} -fcf-protection=none")
ENDIF ()

# DEBUG 参数
IF (DEBUG)
    SET(COMPILE_FLAGS "${COMPILE_FLAGS} -g3 -O3 -ggdb")
ELSE ()
    SET(COMPILE_FLAGS "${COMPILE_FLAGS} -g0 -O3 -fno-stack-protector")
ENDIF ()
