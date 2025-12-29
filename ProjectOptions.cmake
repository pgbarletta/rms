include(cmake/SystemLink.cmake)
include(cmake/LibFuzzer.cmake)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)


include(CheckCXXSourceCompiles)


macro(rms_supports_sanitizers)
  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND NOT WIN32)

    message(STATUS "Sanity checking UndefinedBehaviorSanitizer, it should be supported on this platform")
    set(TEST_PROGRAM "int main() { return 0; }")

    # Check if UndefinedBehaviorSanitizer works at link time
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=undefined")
    set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=undefined")
    check_cxx_source_compiles("${TEST_PROGRAM}" HAS_UBSAN_LINK_SUPPORT)

    if(HAS_UBSAN_LINK_SUPPORT)
      message(STATUS "UndefinedBehaviorSanitizer is supported at both compile and link time.")
      set(SUPPORTS_UBSAN ON)
    else()
      message(WARNING "UndefinedBehaviorSanitizer is NOT supported at link time.")
      set(SUPPORTS_UBSAN OFF)
    endif()
  else()
    set(SUPPORTS_UBSAN OFF)
  endif()

  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND WIN32)
    set(SUPPORTS_ASAN OFF)
  else()
    if (NOT WIN32)
      message(STATUS "Sanity checking AddressSanitizer, it should be supported on this platform")
      set(TEST_PROGRAM "int main() { return 0; }")

      # Check if AddressSanitizer works at link time
      set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
      set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address")
      check_cxx_source_compiles("${TEST_PROGRAM}" HAS_ASAN_LINK_SUPPORT)

      if(HAS_ASAN_LINK_SUPPORT)
        message(STATUS "AddressSanitizer is supported at both compile and link time.")
        set(SUPPORTS_ASAN ON)
      else()
        message(WARNING "AddressSanitizer is NOT supported at link time.")
        set(SUPPORTS_ASAN OFF)
      endif()
    else()
      set(SUPPORTS_ASAN ON)
    endif()
  endif()
endmacro()

macro(rms_setup_options)
  option(rms_ENABLE_HARDENING "Enable hardening" ON)
  option(rms_ENABLE_COVERAGE "Enable coverage reporting" OFF)
  cmake_dependent_option(
    rms_ENABLE_GLOBAL_HARDENING
    "Attempt to push hardening options to built dependencies"
    ON
    rms_ENABLE_HARDENING
    OFF)

  rms_supports_sanitizers()

  if(NOT PROJECT_IS_TOP_LEVEL OR rms_PACKAGING_MAINTAINER_MODE)
    option(rms_ENABLE_IPO "Enable IPO/LTO" OFF)
    option(rms_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
    option(rms_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(rms_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    option(rms_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(rms_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
    option(rms_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(rms_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(rms_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(rms_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
    option(rms_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(rms_ENABLE_PCH "Enable precompiled headers" OFF)
    option(rms_ENABLE_CACHE "Enable ccache" OFF)
  else()
    option(rms_ENABLE_IPO "Enable IPO/LTO" ON)
    option(rms_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
    option(rms_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(rms_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ${SUPPORTS_ASAN})
    option(rms_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(rms_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" ${SUPPORTS_UBSAN})
    option(rms_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(rms_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(rms_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(rms_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
    option(rms_ENABLE_CPPCHECK "Enable cpp-check analysis" ON)
    option(rms_ENABLE_PCH "Enable precompiled headers" OFF)
    option(rms_ENABLE_CACHE "Enable ccache" ON)
  endif()

  if(NOT PROJECT_IS_TOP_LEVEL)
    mark_as_advanced(
      rms_ENABLE_IPO
      rms_WARNINGS_AS_ERRORS
      rms_ENABLE_USER_LINKER
      rms_ENABLE_SANITIZER_ADDRESS
      rms_ENABLE_SANITIZER_LEAK
      rms_ENABLE_SANITIZER_UNDEFINED
      rms_ENABLE_SANITIZER_THREAD
      rms_ENABLE_SANITIZER_MEMORY
      rms_ENABLE_UNITY_BUILD
      rms_ENABLE_CLANG_TIDY
      rms_ENABLE_CPPCHECK
      rms_ENABLE_COVERAGE
      rms_ENABLE_PCH
      rms_ENABLE_CACHE)
  endif()

  rms_check_libfuzzer_support(LIBFUZZER_SUPPORTED)
  if(LIBFUZZER_SUPPORTED AND (rms_ENABLE_SANITIZER_ADDRESS OR rms_ENABLE_SANITIZER_THREAD OR rms_ENABLE_SANITIZER_UNDEFINED))
    set(DEFAULT_FUZZER ON)
  else()
    set(DEFAULT_FUZZER OFF)
  endif()

  option(rms_BUILD_FUZZ_TESTS "Enable fuzz testing executable" ${DEFAULT_FUZZER})

endmacro()

macro(rms_global_options)
  if(rms_ENABLE_IPO)
    include(cmake/InterproceduralOptimization.cmake)
    rms_enable_ipo()
  endif()

  rms_supports_sanitizers()

  if(rms_ENABLE_HARDENING AND rms_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR rms_ENABLE_SANITIZER_UNDEFINED
       OR rms_ENABLE_SANITIZER_ADDRESS
       OR rms_ENABLE_SANITIZER_THREAD
       OR rms_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    message("${rms_ENABLE_HARDENING} ${ENABLE_UBSAN_MINIMAL_RUNTIME} ${rms_ENABLE_SANITIZER_UNDEFINED}")
    rms_enable_hardening(rms_options ON ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()
endmacro()

macro(rms_local_options)
  if(PROJECT_IS_TOP_LEVEL)
    include(cmake/StandardProjectSettings.cmake)
  endif()

  add_library(rms_warnings INTERFACE)
  add_library(rms_options INTERFACE)

  include(cmake/CompilerWarnings.cmake)
  rms_set_project_warnings(
    rms_warnings
    ${rms_WARNINGS_AS_ERRORS}
    ""
    ""
    ""
    "")

  if(rms_ENABLE_USER_LINKER)
    include(cmake/Linker.cmake)
    rms_configure_linker(rms_options)
  endif()

  include(cmake/Sanitizers.cmake)
  rms_enable_sanitizers(
    rms_options
    ${rms_ENABLE_SANITIZER_ADDRESS}
    ${rms_ENABLE_SANITIZER_LEAK}
    ${rms_ENABLE_SANITIZER_UNDEFINED}
    ${rms_ENABLE_SANITIZER_THREAD}
    ${rms_ENABLE_SANITIZER_MEMORY})

  set_target_properties(rms_options PROPERTIES UNITY_BUILD ${rms_ENABLE_UNITY_BUILD})

  if(rms_ENABLE_PCH)
    target_precompile_headers(
      rms_options
      INTERFACE
      <vector>
      <string>
      <utility>)
  endif()

  if(rms_ENABLE_CACHE)
    include(cmake/Cache.cmake)
    rms_enable_cache()
  endif()

  include(cmake/StaticAnalyzers.cmake)
  if(rms_ENABLE_CLANG_TIDY)
    rms_enable_clang_tidy(rms_options ${rms_WARNINGS_AS_ERRORS})
  endif()

  if(rms_ENABLE_CPPCHECK)
    rms_enable_cppcheck(${rms_WARNINGS_AS_ERRORS} "" # override cppcheck options
    )
  endif()

  if(rms_ENABLE_COVERAGE)
    include(cmake/Tests.cmake)
    rms_enable_coverage(rms_options)
  endif()

  if(rms_WARNINGS_AS_ERRORS)
    check_cxx_compiler_flag("-Wl,--fatal-warnings" LINKER_FATAL_WARNINGS)
    if(LINKER_FATAL_WARNINGS)
      # This is not working consistently, so disabling for now
      # target_link_options(rms_options INTERFACE -Wl,--fatal-warnings)
    endif()
  endif()

  if(rms_ENABLE_HARDENING AND NOT rms_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR rms_ENABLE_SANITIZER_UNDEFINED
       OR rms_ENABLE_SANITIZER_ADDRESS
       OR rms_ENABLE_SANITIZER_THREAD
       OR rms_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    rms_enable_hardening(rms_options OFF ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()

endmacro()
