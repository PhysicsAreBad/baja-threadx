# Toolchain file for the Arm GNU Toolchain (arm-none-eabi-gcc).
# If the toolchain is not on PATH, pass -DARM_TOOLCHAIN_PATH=/path/to/toolchain/bin
# or set the ARM_TOOLCHAIN_PATH environment variable.

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

if(NOT ARM_TOOLCHAIN_PATH AND DEFINED ENV{ARM_TOOLCHAIN_PATH})
    set(ARM_TOOLCHAIN_PATH $ENV{ARM_TOOLCHAIN_PATH})
endif()
if(ARM_TOOLCHAIN_PATH)
    set(_tc_prefix "${ARM_TOOLCHAIN_PATH}/arm-none-eabi-")
else()
    set(_tc_prefix "arm-none-eabi-")
endif()
# Make it visible in try_compile() projects too.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES ARM_TOOLCHAIN_PATH)

set(CMAKE_C_COMPILER   ${_tc_prefix}gcc)
set(CMAKE_CXX_COMPILER ${_tc_prefix}g++)
set(CMAKE_ASM_COMPILER ${_tc_prefix}gcc)
set(CMAKE_OBJCOPY      ${_tc_prefix}objcopy CACHE FILEPATH "")
set(CMAKE_SIZE         ${_tc_prefix}size    CACHE FILEPATH "")

# Linking a full executable needs a linker script, so test with a static lib.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
