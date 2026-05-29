cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Compiler options
set(STM32_MCU_FLAGS  "-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard " )

#Linker options
set(STM32_LINKER_SCRIPT stm32h747xx_flash_CM7.ld)
set(STM32_LINKER_OPTION  )

# Set the project name
set(CMAKE_PROJECT_NAME qiansai_CM7)
project(${CMAKE_PROJECT_NAME})

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Include mx-generated
include("mx-generated.cmake")


# In order to use CMake for cross-compiling
set(CMAKE_CXX_COMPILER_FORCED "true")

# Compiler options: disable thread-safe statics (bare-metal — not needed, may use unsupported instructions)
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Enable CMake support for ASM, C and C++ languages
enable_language(C ASM CXX)

# ---- TouchGFX paths ----
set(TOUCHGFX_ROOT "E:/TouchGFX/4.26.1")
set(TOUCHGFX_FRAMEWORK "${TOUCHGFX_ROOT}/touchgfx/framework")
set(TOUCHGFX_LIB_CORE "${TOUCHGFX_ROOT}/touchgfx/lib/core/cortex_m7/gcc")

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # User defined include paths
    Core/Inc
    Core/Hardware
    Drivers/STM32H7xx_HAL_Driver/Inc
    Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
    # TouchGFX framework
    ${TOUCHGFX_FRAMEWORK}/include
    # TouchGFX HAL and GUI
    .
    TouchGFX
    TouchGFX/target
    TouchGFX/gui
    TouchGFX/gui/include
    TouchGFX/generated
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # User crash handler (enhanced fault diagnostics)
    Core/Src/crash_handler.c
    # TouchGFX license check override (CRC_Lock always returns success)
    Core/Src/crc_lock_override.c
    # FreeRTOS heap in SDRAM (configAPPLICATION_ALLOCATED_HEAP=1)
    Core/Src/freertos_heap_sdram.c
    # User hardware sources
    Core/Hardware/sys.c
    Core/Hardware/delay.c
    Core/Hardware/mpu.c
    Core/Hardware/sdram.c
    Core/Hardware/ltdc.c
    # TouchGFX target HAL
    TouchGFX/target/TouchGFXHAL.cpp
    TouchGFX/target/TouchGFXConfiguration.cpp
    TouchGFX/target/OSWrappers.cpp
    TouchGFX/target/GPIO.cpp
    TouchGFX/target/PaintRGB565.cpp
    # TouchGFX GUI
    TouchGFX/gui/screen1_screen/Screen1View.cpp
    TouchGFX/gui/src/common/FrontendApplication.cpp
)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${TOUCHGFX_LIB_CORE}
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    touchgfx-float-abi-hard
    -Wl,--wrap=CRC_Lock
)

# ---- TouchGFX Framework compilation ----
file(GLOB_RECURSE TOUCHGFX_FRAMEWORK_SRC
    "${TOUCHGFX_FRAMEWORK}/source/touchgfx/**/*.cpp"
)

add_library(TouchGFX_Framework OBJECT ${TOUCHGFX_FRAMEWORK_SRC})
target_include_directories(TouchGFX_Framework PRIVATE
    ${TOUCHGFX_FRAMEWORK}/include
)
target_compile_definitions(TouchGFX_Framework PRIVATE
    STM32H747xx
    USE_HAL_DRIVER
    CORE_CM7
)
target_link_libraries(${CMAKE_PROJECT_NAME} TouchGFX_Framework)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()
