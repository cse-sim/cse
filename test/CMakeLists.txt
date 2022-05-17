add_subdirectory(wcmp)

macro(run_test case)
  get_filename_component(test_name ${case} NAME_WE)
  set(test_comment "! ")
  add_test(NAME ${test_name}.Run
    COMMAND $<TARGET_FILE:CSE> -x${test_comment} -b -t1 "${test_name}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  )
endmacro()

macro(regress_test case)
  get_filename_component(test_name ${case} NAME_WE)
  add_test(NAME ${test_name}.Regression
    COMMAND $<TARGET_FILE:wcmp> "${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.rep" "${CMAKE_CURRENT_SOURCE_DIR}/ref/${test_name}.rep" "!"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  set_tests_properties(${test_name}.Regression PROPERTIES DEPENDS ${test_name}.Run)
endmacro()

set(files
  wthr01.CSE
  600.cse
  930.cse
  960.cse
  2Zone.cse
  3ZoneAirNet.cse
  3ZoneAirNetCSW.cse
  Fresno.cse
  Autosize.cse
  masstest1.cse
  # 12P21M
  # 1ZCZMU
  Z2SG.cse
  zonetest4W.cse
  zt5d.cse
  qw5ds.cse
  izfantest.cse
  # 2700x
  2zun.cse
  1zattic.cse
  1ZKiva.cse
  2zattic.cse
  actest1bL.cse
  ashptest2
  ashpPkgRoom
  ashp_dfng
  ashpvc2
  fancoil
  wshp
  herv
  Multifamily-DOAS
  oavtest2
  bgtest
  ashwat1
  loadAndAFMeters
  dhw02
  dhwX
  dhwDU
  dhw_C
  dhw_solar
  dhw_zone
  dwhr
  dhwls
  DHW_INV 
  dhw_uef
  dhw_brwl
  dhwloop32U
  dhw_mfsizing
  pvtest
  pv_bt_test
  bt_test
  shadetest
  CRTerminalAH
  MF8X
)

foreach(file ${files})
  run_test(${file})
  regress_test(${file})
endforeach()

configure_file("diff.bat.in" "${CMAKE_BINARY_DIR}/diff.bat")
configure_file("update.bat.in" "${CMAKE_BINARY_DIR}/update.bat")


