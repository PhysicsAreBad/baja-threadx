cmake_minimum_required(VERSION 3.22)

if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/arm-none-eabi.cmake)
endif()

project(stm32h563_adc_walkthrough C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(THIRD_PARTY ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
if(NOT EXISTS ${THIRD_PARTY}/threadx/common/src/tx_thread_create.c)
    message(FATAL_ERROR "Submodules missing. Submodules missing. Run: git submodule update --init --recursive")
endif()

# ---------------------------------------------------------------------------
# MCU flags / common settings (applied to every target below)
# ---------------------------------------------------------------------------
set(MCU_FLAGS -mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard)

add_library(mcu_settings INTERFACE)
target_compile_options(mcu_settings INTERFACE
    ${MCU_FLAGS}
    -ffunction-sections -fdata-sections
    $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra>
    $<$<COMPILE_LANGUAGE:ASM>:-x assembler-with-cpp>
    $<$<CONFIG:Debug>:-Og -g3>
    $<$<CONFIG:Release>:-O2 -g>
)
target_compile_definitions(mcu_settings INTERFACE
    STM32H563xx
    USE_HAL_DRIVER
    TX_INCLUDE_USER_DEFINE_FILE
    TX_SINGLE_MODE_NON_SECURE=1   # TrustZone disabled (TZEN=0, the factory default)
    $<$<CONFIG:Debug>:DEBUG>
)
target_include_directories(mcu_settings INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${THIRD_PARTY}/cmsis_core/Include
    ${THIRD_PARTY}/cmsis_device_h5/Include
    ${THIRD_PARTY}/stm32h5xx_hal_driver/Inc
)

# ---------------------------------------------------------------------------
# ThreadX (Cortex-M33 GNU port). The port's generic tx_initialize_low_level.S
# is replaced by Core/Startup/tx_initialize_low_level.S.
# ---------------------------------------------------------------------------
set(TX_PORT_DIR ${THIRD_PARTY}/threadx/ports/cortex_m33/gnu)
file(GLOB THREADX_SOURCES
    ${THIRD_PARTY}/threadx/common/src/*.c
    ${TX_PORT_DIR}/src/*.c
    ${TX_PORT_DIR}/src/*.S
)
list(FILTER THREADX_SOURCES EXCLUDE REGEX "/(tx_initialize_low_level|tx_misra)\\.S$")

add_library(threadx STATIC ${THREADX_SOURCES})
target_include_directories(threadx PUBLIC
    ${THIRD_PARTY}/threadx/common/inc
    ${TX_PORT_DIR}/inc
)
target_link_libraries(threadx PUBLIC mcu_settings)
target_compile_options(threadx PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-unused-parameter>)

# ---------------------------------------------------------------------------
# STM32H5 HAL. Every HAL source is built; modules not enabled in
# Core/Inc/stm32h5xx_hal_conf.h compile to nothing.
# ---------------------------------------------------------------------------
file(GLOB HAL_SOURCES ${THIRD_PARTY}/stm32h5xx_hal_driver/Src/stm32h5xx_hal*.c)
list(FILTER HAL_SOURCES EXCLUDE REGEX "_template\\.c$")

add_library(stm32h5_hal STATIC ${HAL_SOURCES})
target_link_libraries(stm32h5_hal PUBLIC mcu_settings)
target_compile_options(stm32h5_hal PRIVATE -Wno-unused-parameter)

# ---------------------------------------------------------------------------
# Application
# ---------------------------------------------------------------------------
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/STM32H563xx_FLASH.ld)

add_executable(${PROJECT_NAME}
    Core/Startup/startup_stm32h563xx.s
    Core/Startup/tx_initialize_low_level.S
    Core/Src/system_stm32h5xx.c
    Core/Src/main.c
    Core/Src/app_threadx.c
    Core/Src/adc_sensor.c
    Core/Src/stm32h5xx_it.c
    Core/Src/stm32h5xx_hal_timebase_tim.c
    Core/Src/syscalls.c
)
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX .elf LINK_DEPENDS ${LINKER_SCRIPT})
target_link_libraries(${PROJECT_NAME} PRIVATE threadx stm32h5_hal mcu_settings)
target_link_options(${PROJECT_NAME} PRIVATE
    ${MCU_FLAGS}
    -T${LINKER_SCRIPT}
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.map
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

# `cmake --build build --target flash` (needs STM32CubeProgrammer on PATH)
find_program(STM32_PROGRAMMER_CLI STM32_Programmer_CLI)
if(STM32_PROGRAMMER_CLI)
    add_custom_target(flash
        COMMAND ${STM32_PROGRAMMER_CLI} -c port=SWD mode=UR -w $<TARGET_FILE:${PROJECT_NAME}> -v -rst
        DEPENDS ${PROJECT_NAME}
        USES_TERMINAL
    )
endif()
