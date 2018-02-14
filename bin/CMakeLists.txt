add_custom_target(ssc ALL
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/ssc.dll "${CMAKE_SOURCE_DIR}/msvc/ssc.dll"
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ssc.dll
)

set_target_properties(ssc PROPERTIES FOLDER Dependencies)
