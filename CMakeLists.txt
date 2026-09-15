cmake_minimum_required(VERSION 3.24)

project(openpnp_motion_controller LANGUAGES C ASM)

include(FetchContent)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(OPENPNP_FDCAN_MODE "loopback" CACHE STRING "FDCAN bring-up mode: loopback or normal")
set_property(CACHE OPENPNP_FDCAN_MODE PROPERTY STRINGS loopback normal)
if(OPENPNP_FDCAN_MODE STREQUAL "loopback")
  set(OPENPNP_FDCAN_INTERNAL_LOOPBACK 1)
elseif(OPENPNP_FDCAN_MODE STREQUAL "normal")
  set(OPENPNP_FDCAN_INTERNAL_LOOPBACK 0)
else()
  message(FATAL_ERROR "OPENPNP_FDCAN_MODE must be 'loopback' or 'normal'")
endif()

set(CMSIS_CORE_ROOT "" CACHE PATH "Path to an existing CMSIS_5 checkout")
set(CMSIS_DEVICE_G4_ROOT "" CACHE PATH "Path to an existing cmsis-device-g4 checkout")
set(STM32G4_HAL_ROOT "" CACHE PATH "Path to an existing stm32g4xx-hal-driver checkout")

# Keep dependency revisions immutable. These commits are the peeled commits for
# CMSIS_5 5.9.0 and cmsis-device-g4 v1.2.6 respectively.
set(CMSIS_CORE_COMMIT "2b7495b8535bdcb306dac29b9ded4cfb679d7e5c")
set(CMSIS_DEVICE_G4_COMMIT "25664ddc3a7624ae9627ae8c4c672073dc5b2539")
set(STM32G4_HAL_COMMIT "6940aef00ac466305872e4153f41b52dc954d6cf")

if(NOT CMSIS_CORE_ROOT)
  FetchContent_Declare(
    cmsis_core
    GIT_REPOSITORY https://github.com/ARM-software/CMSIS_5.git
    GIT_TAG ${CMSIS_CORE_COMMIT}
    GIT_PROGRESS TRUE
    SOURCE_SUBDIR _not_used_by_this_project
  )
  FetchContent_MakeAvailable(cmsis_core)
  set(CMSIS_CORE_ROOT "${cmsis_core_SOURCE_DIR}")
endif()

if(NOT CMSIS_DEVICE_G4_ROOT)
  FetchContent_Declare(
    cmsis_device_g4
    GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis-device-g4.git
    GIT_TAG ${CMSIS_DEVICE_G4_COMMIT}
    GIT_PROGRESS TRUE
    SOURCE_SUBDIR _not_used_by_this_project
  )
  FetchContent_MakeAvailable(cmsis_device_g4)
  set(CMSIS_DEVICE_G4_ROOT "${cmsis_device_g4_SOURCE_DIR}")
endif()

if(NOT STM32G4_HAL_ROOT)
  FetchContent_Declare(
    stm32g4_hal
    GIT_REPOSITORY https://github.com/STMicroelectronics/stm32g4xx-hal-driver.git
    GIT_TAG ${STM32G4_HAL_COMMIT}
    GIT_PROGRESS TRUE
    SOURCE_SUBDIR _not_used_by_this_project
  )
  FetchContent_MakeAvailable(stm32g4_hal)
  set(STM32G4_HAL_ROOT "${stm32g4_hal_SOURCE_DIR}")
endif()

set(CMSIS_CORE_INCLUDE "${CMSIS_CORE_ROOT}/CMSIS/Core/Include")
set(CMSIS_DEVICE_G4_INCLUDE "${CMSIS_DEVICE_G4_ROOT}/Include")
set(CMSIS_DEVICE_G4_TEMPLATES "${CMSIS_DEVICE_G4_ROOT}/Source/Templates")
set(STM32G4_HAL_INCLUDE "${STM32G4_HAL_ROOT}/Inc")
set(STM32G4_HAL_SOURCE "${STM32G4_HAL_ROOT}/Src")

foreach(required_path IN ITEMS
    "${CMSIS_CORE_INCLUDE}/core_cm4.h"
    "${CMSIS_DEVICE_G4_INCLUDE}/stm32g431xx.h"
    "${CMSIS_DEVICE_G4_TEMPLATES}/system_stm32g4xx.c"
    "${CMSIS_DEVICE_G4_TEMPLATES}/gcc/startup_stm32g431xx.s"
    "${STM32G4_HAL_INCLUDE}/stm32g4xx_hal_fdcan.h"
    "${STM32G4_HAL_SOURCE}/stm32g4xx_hal_fdcan.c")
  if(NOT EXISTS "${required_path}")
    message(FATAL_ERROR "Required CMSIS file is missing: ${required_path}")
  endif()
endforeach()

set(FIRMWARE_NAME "openpnp-motion-stm32g431cb-${OPENPNP_FDCAN_MODE}")
set(ARTIFACT_DIR "${CMAKE_BINARY_DIR}/artifacts")
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/firmware/linker/STM32G431CBTx_FLASH.ld")

file(MAKE_DIRECTORY "${ARTIFACT_DIR}")

add_executable(firmware
  firmware/src/axis_runtime_stm32g431.c
  firmware/src/clock_sync.c
  firmware/src/fdcan_port_stm32g431.c
  firmware/src/interrupts.c
  firmware/src/main.c
  firmware/src/motion_clock_stm32g431.c
  firmware/src/platform_stm32g431.c
  firmware/src/step_engine.c
  firmware/src/syscalls.c
  firmware/src/target_contract.c
  "${STM32G4_HAL_SOURCE}/stm32g4xx_hal.c"
  "${STM32G4_HAL_SOURCE}/stm32g4xx_hal_cortex.c"
  "${STM32G4_HAL_SOURCE}/stm32g4xx_hal_fdcan.c"
  "${STM32G4_HAL_SOURCE}/stm32g4xx_hal_gpio.c"
  "${STM32G4_HAL_SOURCE}/stm32g4xx_hal_pwr.c"
  "${STM32G4_HAL_SOURCE}/stm32g4xx_hal_pwr_ex.c"
  "${STM32G4_HAL_SOURCE}/stm32g4xx_hal_rcc.c"
  "${STM32G4_HAL_SOURCE}/stm32g4xx_hal_rcc_ex.c"
  "${CMSIS_DEVICE_G4_TEMPLATES}/system_stm32g4xx.c"
  "${CMSIS_DEVICE_G4_TEMPLATES}/gcc/startup_stm32g431xx.s"
)

set_target_properties(firmware PROPERTIES
  OUTPUT_NAME "${FIRMWARE_NAME}"
  SUFFIX ".elf"
  RUNTIME_OUTPUT_DIRECTORY "${ARTIFACT_DIR}"
)

target_include_directories(firmware PRIVATE
  firmware/include
  "${CMSIS_CORE_INCLUDE}"
  "${CMSIS_DEVICE_G4_INCLUDE}"
  "${STM32G4_HAL_INCLUDE}"
)

target_compile_definitions(firmware PRIVATE
  STM32G431xx
  OPENPNP_FDCAN_INTERNAL_LOOPBACK=${OPENPNP_FDCAN_INTERNAL_LOOPBACK}
)

set(CPU_FLAGS
  -mcpu=cortex-m4
  -mthumb
  -mfpu=fpv4-sp-d16
  -mfloat-abi=hard
)

target_compile_options(firmware PRIVATE
  ${CPU_FLAGS}
  -ffunction-sections
  -fdata-sections
  -fno-common
  -Wall
  -Wextra
  -Wpedantic
  -Werror
  $<$<CONFIG:Debug>:-Og;-g3>
  $<$<CONFIG:Release>:-Os;-g1>
)

target_link_options(firmware PRIVATE
  ${CPU_FLAGS}
  -T${LINKER_SCRIPT}
  --specs=nano.specs
  --specs=nosys.specs
  -Wl,--gc-sections
  -Wl,--entry=Reset_Handler
  -Wl,--fatal-warnings
  -Wl,--no-warn-rwx-segment
  -Wl,-Map,${ARTIFACT_DIR}/${FIRMWARE_NAME}.map,--cref
)

add_custom_command(TARGET firmware POST_BUILD
  COMMAND "${CMAKE_OBJCOPY}" -O ihex
          "$<TARGET_FILE:firmware>" "${ARTIFACT_DIR}/${FIRMWARE_NAME}.hex"
  COMMAND "${CMAKE_OBJCOPY}" -O binary
          "$<TARGET_FILE:firmware>" "${ARTIFACT_DIR}/${FIRMWARE_NAME}.bin"
  COMMAND "${CMAKE_COMMAND}"
          -DSIZE_TOOL=${CMAKE_SIZE}
          -DINPUT=$<TARGET_FILE:firmware>
          -DOUTPUT=${ARTIFACT_DIR}/${FIRMWARE_NAME}-size.txt
          -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/write_size.cmake"
  COMMAND "${CMAKE_COMMAND}"
          -DOBJDUMP_TOOL=${CMAKE_OBJDUMP}
          -DNM_TOOL=${CMAKE_NM}
          -DINPUT=$<TARGET_FILE:firmware>
          -DBINARY=${ARTIFACT_DIR}/${FIRMWARE_NAME}.bin
          -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/verify_image.cmake"
  COMMENT "Generating and verifying STM32G431CBT6 firmware artifacts"
  VERBATIM
)

configure_file(
  cmake/build-info.txt.in
  "${ARTIFACT_DIR}/${FIRMWARE_NAME}-build-info.txt"
  @ONLY
)

configure_file(
  LICENSE
  "${ARTIFACT_DIR}/LICENSE-project.txt"
  COPYONLY
)
configure_file(
  "${CMSIS_CORE_ROOT}/LICENSE.txt"
  "${ARTIFACT_DIR}/LICENSE-CMSIS-5.txt"
  COPYONLY
)
configure_file(
  "${CMSIS_DEVICE_G4_ROOT}/LICENSE.md"
  "${ARTIFACT_DIR}/LICENSE-CMSIS-Device-G4.txt"
  COPYONLY
)
configure_file(
  "${STM32G4_HAL_ROOT}/LICENSE.md"
  "${ARTIFACT_DIR}/LICENSE-STM32G4-HAL.txt"
  COPYONLY
)
