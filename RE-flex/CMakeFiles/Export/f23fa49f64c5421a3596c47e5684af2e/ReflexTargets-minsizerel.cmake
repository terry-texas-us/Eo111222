#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Reflex::Reflex" for configuration "MinSizeRel"
set_property(TARGET Reflex::Reflex APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(Reflex::Reflex PROPERTIES
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/bin/reflex.exe"
  )

list(APPEND _cmake_import_check_targets Reflex::Reflex )
list(APPEND _cmake_import_check_files_for_Reflex::Reflex "${_IMPORT_PREFIX}/bin/reflex.exe" )

# Import target "Reflex::ReflexLib" for configuration "MinSizeRel"
set_property(TARGET Reflex::ReflexLib APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(Reflex::ReflexLib PROPERTIES
  IMPORTED_IMPLIB_MINSIZEREL "${_IMPORT_PREFIX}/lib/reflex_shared_lib.lib"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/bin/reflex_shared_lib.dll"
  )

list(APPEND _cmake_import_check_targets Reflex::ReflexLib )
list(APPEND _cmake_import_check_files_for_Reflex::ReflexLib "${_IMPORT_PREFIX}/lib/reflex_shared_lib.lib" "${_IMPORT_PREFIX}/bin/reflex_shared_lib.dll" )

# Import target "Reflex::ReflexLibStatic" for configuration "MinSizeRel"
set_property(TARGET Reflex::ReflexLibStatic APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(Reflex::ReflexLibStatic PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "CXX"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/reflex_static_lib.lib"
  )

list(APPEND _cmake_import_check_targets Reflex::ReflexLibStatic )
list(APPEND _cmake_import_check_files_for_Reflex::ReflexLibStatic "${_IMPORT_PREFIX}/lib/reflex_static_lib.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
