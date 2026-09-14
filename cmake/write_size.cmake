foreach(required_variable IN ITEMS SIZE_TOOL INPUT OUTPUT)
  if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
    message(FATAL_ERROR "${required_variable} must be provided")
  endif()
endforeach()

execute_process(
  COMMAND "${SIZE_TOOL}" --format=berkeley "${INPUT}"
  RESULT_VARIABLE size_result
  OUTPUT_VARIABLE size_output
  ERROR_VARIABLE size_error
)

if(NOT size_result EQUAL 0)
  message(FATAL_ERROR "arm-none-eabi-size failed: ${size_error}")
endif()

file(WRITE "${OUTPUT}" "${size_output}")
string(STRIP "${size_output}" size_output_stripped)
message(STATUS "Firmware size:\n${size_output_stripped}")

