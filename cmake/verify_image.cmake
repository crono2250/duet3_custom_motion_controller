foreach(required_variable IN ITEMS OBJDUMP_TOOL NM_TOOL INPUT BINARY)
  if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
    message(FATAL_ERROR "${required_variable} must be provided")
  endif()
endforeach()

execute_process(
  COMMAND "${OBJDUMP_TOOL}" -h "${INPUT}"
  RESULT_VARIABLE objdump_result
  OUTPUT_VARIABLE section_table
  ERROR_VARIABLE objdump_error
)

if(NOT objdump_result EQUAL 0)
  message(FATAL_ERROR "arm-none-eabi-objdump failed: ${objdump_error}")
endif()

string(REGEX MATCH
  "\\.isr_vector[ \t]+[0-9a-fA-F]+[ \t]+08000000[ \t]+08000000"
  vector_match
  "${section_table}"
)

if(NOT vector_match)
  message(FATAL_ERROR "The interrupt vector table is not linked at 0x08000000")
endif()

execute_process(
  COMMAND "${NM_TOOL}" --defined-only "${INPUT}"
  RESULT_VARIABLE nm_result
  OUTPUT_VARIABLE symbol_table
  ERROR_VARIABLE nm_error
)

if(NOT nm_result EQUAL 0)
  message(FATAL_ERROR "arm-none-eabi-nm failed: ${nm_error}")
endif()

foreach(required_symbol IN ITEMS Reset_Handler main g_target_contract g_bringup_diagnostics TIM2_IRQHandler SysTick_Handler FDCAN1_IT0_IRQHandler)
  if(NOT symbol_table MATCHES "[ \t]${required_symbol}")
    message(FATAL_ERROR "Required symbol is missing from image: ${required_symbol}")
  endif()
endforeach()

if(NOT EXISTS "${BINARY}")
  message(FATAL_ERROR "Binary artifact was not generated: ${BINARY}")
endif()

file(SIZE "${BINARY}" binary_size)
if(binary_size LESS 1)
  message(FATAL_ERROR "Binary artifact is empty")
endif()

message(STATUS "Image verification passed (${binary_size} byte binary)")
