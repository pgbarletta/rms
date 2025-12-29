add_library(rms_parm7)
add_library(rms::parm7 ALIAS rms_parm7)

target_sources(rms_parm7
  PRIVATE
    forcefield.cpp
    parsers.cpp
    include/parsers.hpp
    include/forcefield.hpp
    include/utils.hpp
)

target_link_libraries(rms_parm7
  PUBLIC
    rms::rms_options
    rms::rms_warnings
  PRIVATE
    fmt::fmt
)

target_include_directories(rms_parm7
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_executable(rms
  main.cpp
  cli.cpp
  include/cli.hpp
)

target_link_libraries(rms
  PRIVATE
    rms::parm7
    rms::rms_options
    rms::rms_warnings
)

target_link_system_libraries(rms
  PRIVATE
    CLI11::CLI11
    fmt::fmt
)

target_include_directories(rms PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")

add_executable(rms_parm7_bench
  bench_parm7.cpp
)

target_link_libraries(rms_parm7_bench
  PRIVATE
    rms::parm7
    rms::rms_options
    rms::rms_warnings
    fmt::fmt
)
