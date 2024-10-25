#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Boost::unit_test_framework" for configuration "Release"
set_property(TARGET Boost::unit_test_framework APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Boost::unit_test_framework PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/boost_unit_test_framework-vc143-mt-x64-1_86.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/boost_unit_test_framework-vc143-mt-x64-1_86.dll"
  )

list(APPEND _cmake_import_check_targets Boost::unit_test_framework )
list(APPEND _cmake_import_check_files_for_Boost::unit_test_framework "${_IMPORT_PREFIX}/lib/boost_unit_test_framework-vc143-mt-x64-1_86.lib" "${_IMPORT_PREFIX}/bin/boost_unit_test_framework-vc143-mt-x64-1_86.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
