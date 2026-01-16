# INCLUDE : include(Gzip)
# COMMAND : compress_gz(-9 ${INPUT_FILE} ${OUTPUT_FILE})
################################################################################

find_program(GZIP gzip)

if(GZIP)
  message(STATUS "Found gzip: ${GZIP}")

  # It is not possible to add a dependency to target 'all'
  # Run hard-coded 'make gzip' when 'make install' is invoked
  install(CODE "EXECUTE_PROCESS(COMMAND make gzip)")
  add_custom_target(gzip)

  macro(compress_gz COMPRESSION_RATE INPUT_FILE OUTPUT_FILE)
    get_filename_component(FILENAME "${INPUT_FILE}" NAME)
    get_filename_component(OUTPUT_PATH "${OUTPUT_FILE}" DIRECTORY)

    add_custom_command(
      OUTPUT "${OUTPUT_FILE}"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_PATH}"
      COMMAND ${GZIP} ${COMPRESSION_RATE} -c "${INPUT_FILE}" > "${OUTPUT_FILE}")

    add_custom_target("gzip_${FILENAME}"
      DEPENDS "${OUTPUT_FILE}")

    add_dependencies(gzip "gzip_${FILENAME}")
  endmacro()
else()
  message(STATUS "Not found gzip: Files.gz won't be compressed")
  macro(compress_gz COMPRESSION_RATE INPUT_FILE OUTPUT_FILE)
  endmacro()
endif()

