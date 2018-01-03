add_custom_target(ssc ALL
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/ssc.dll "${CMAKE_BINARY_DIR}/products/ssc.dll"
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ssc.dll
)
