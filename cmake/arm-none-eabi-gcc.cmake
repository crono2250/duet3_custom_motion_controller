set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_program(CMAKE_C_COMPILER NAMES arm-none-eabi-gcc REQUIRED)
set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}")

find_program(CMAKE_AR NAMES arm-none-eabi-ar REQUIRED)
find_program(CMAKE_NM NAMES arm-none-eabi-nm REQUIRED)
find_program(CMAKE_OBJCOPY NAMES arm-none-eabi-objcopy REQUIRED)
find_program(CMAKE_OBJDUMP NAMES arm-none-eabi-objdump REQUIRED)
find_program(CMAKE_RANLIB NAMES arm-none-eabi-ranlib REQUIRED)
find_program(CMAKE_SIZE NAMES arm-none-eabi-size REQUIRED)

set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_ASM_COMPILER_WORKS TRUE)

