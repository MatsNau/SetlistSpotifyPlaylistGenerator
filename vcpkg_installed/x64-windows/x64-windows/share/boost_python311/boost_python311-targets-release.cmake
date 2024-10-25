#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Boost::python311" for configuration "Release"
set_property(TARGET Boost::python311 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Boost::python311 PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/boost_python311-vc143-mt-x64-1_86.lib"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "Boost::graph"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/boost_python311-vc143-mt-x64-1_86.dll"
  )

list(APPEND _cmake_import_check_targets Boost::python311 )
list(APPEND _cmake_import_check_files_for_Boost::python311 "${_IMPORT_PREFIX}/lib/boost_python311-vc143-mt-x64-1_86.lib" "${_IMPORT_PREFIX}/bin/boost_python311-vc143-mt-x64-1_86.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
