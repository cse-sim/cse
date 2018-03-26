add_subdirectory(wcmp)

macro(run_test case)
set(test_comment "! ")
  add_test(NAME ${case}.Run
    COMMAND $<TARGET_FILE:CSE> -x${test_comment} -t1 "${case}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  )
endmacro()

macro(regress_test case)
  add_test(NAME ${case}.Regression
    COMMAND $<TARGET_FILE:wcmp> "${CMAKE_CURRENT_SOURCE_DIR}/${case}.rep" "${CMAKE_CURRENT_SOURCE_DIR}/ref/${case}.rep" "!"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  set_tests_properties(${case}.Regression PROPERTIES DEPENDS ${case}.Run)
endmacro()

set(files
  600
  930
  960
  2Zone
  3ZoneAirNet
  3ZoneAirNetCSW
  Fresno
  Autosize
  masstest1
  # 12P21M
  # 1ZCZMU
  Z2SG
  zonetest4W
  zt5d
  qw5ds
  izfantest
  # 2700x
  2zun
  1zattic
  1ZKiva
  2zattic
  actest1bL
  ashptest2
  herv
  oavtest2
  bgtest
  ashwat1
  dhw02
  dhwX
  dhwDU
  dhw_C
  pvtest
  pv_bt_test
  bt_test
  shadetest
)

foreach(file ${files})
  run_test(${file})
  regress_test(${file})
endforeach()

configure_file("diff.bat.in" "${CMAKE_BINARY_DIR}/diff.bat")

set_target_properties(results_directory PROPERTIES FOLDER Dependencies/Test)
