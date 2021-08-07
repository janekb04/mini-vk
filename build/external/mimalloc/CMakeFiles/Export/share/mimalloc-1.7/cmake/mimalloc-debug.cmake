#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "mimalloc" for configuration "Debug"
set_property(TARGET mimalloc APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(mimalloc PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/mimalloc-1.7/mimalloc-debug.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/mimalloc-1.7/mimalloc-debug.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS mimalloc )
list(APPEND _IMPORT_CHECK_FILES_FOR_mimalloc "${_IMPORT_PREFIX}/lib/mimalloc-1.7/mimalloc-debug.lib" "${_IMPORT_PREFIX}/lib/mimalloc-1.7/mimalloc-debug.dll" )

# Import target "mimalloc-static" for configuration "Debug"
set_property(TARGET mimalloc-static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(mimalloc-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/mimalloc-1.7/mimalloc-static-debug.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS mimalloc-static )
list(APPEND _IMPORT_CHECK_FILES_FOR_mimalloc-static "${_IMPORT_PREFIX}/lib/mimalloc-1.7/mimalloc-static-debug.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
