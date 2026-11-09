cmake_minimum_required(VERSION 3.20)

project(zucchini_standalone VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ZUCCHINI_SOURCE_DIR "" CACHE PATH
    "Path to the AOSP external/zucchini directory")

if(NOT ZUCCHINI_SOURCE_DIR AND DEFINED ENV{AOSP_ROOT})
  set(ZUCCHINI_SOURCE_DIR "$ENV{AOSP_ROOT}/external/zucchini")
endif()

if(NOT ZUCCHINI_SOURCE_DIR)
  message(FATAL_ERROR
    "Set -DZUCCHINI_SOURCE_DIR=/path/to/aosp/external/zucchini "
    "or export AOSP_ROOT=/path/to/aosp")
endif()

get_filename_component(ZUCCHINI_SOURCE_DIR "${ZUCCHINI_SOURCE_DIR}" ABSOLUTE)

if(NOT EXISTS "${ZUCCHINI_SOURCE_DIR}/zucchini_gen.cc")
  message(FATAL_ERROR
    "${ZUCCHINI_SOURCE_DIR} does not look like external/zucchini")
endif()

if(NOT EXISTS "${ZUCCHINI_SOURCE_DIR}/aosp/include/components/zucchini/zucchini.h")
  message(FATAL_ERROR
    "The expected AOSP compatibility headers are missing under aosp/include")
endif()

option(ZUCCHINI_ENABLE_DEX "Build the DEX disassembler" ON)
option(ZUCCHINI_ENABLE_ELF "Build the ELF disassemblers" ON)
option(ZUCCHINI_BUILD_SHARED "Build libzucchini.so" ON)
option(ZUCCHINI_BUILD_STATIC "Build libzucchini.a" ON)
option(ZUCCHINI_BUILD_CLI "Build the standalone zucchini command" ON)

if(NOT ZUCCHINI_ENABLE_DEX AND NOT ZUCCHINI_ENABLE_ELF)
  message(WARNING "Both DEX and ELF support are disabled; only raw/no-op patching remains")
endif()

set(ZUCCHINI_COMMON_SOURCES
  abs32_utils.cc
  address_translator.cc
  arm_utils.cc
  binary_data_histogram.cc
  buffer_sink.cc
  buffer_source.cc
  crc32.cc
  disassembler.cc
  disassembler_no_op.cc
  element_detection.cc
  encoded_view.cc
  ensemble_matcher.cc
  equivalence_map.cc
  heuristic_ensemble_matcher.cc
  image_index.cc
  imposed_ensemble_matcher.cc
  io_utils.cc
  patch_reader.cc
  patch_writer.cc
  reference_bytes_mixer.cc
  reference_set.cc
  target_pool.cc
  targets_affinity.cc
  zucchini_apply.cc
  zucchini_gen.cc
)

set(ZUCCHINI_FORMAT_SOURCES)

add_library(libsais STATIC libsais/src/libsais.c)
target_include_directories(libsais PUBLIC libsais/include)

if(ZUCCHINI_ENABLE_DEX)
  list(APPEND ZUCCHINI_FORMAT_SOURCES disassembler_dex.cc)
endif()

if(ZUCCHINI_ENABLE_ELF)
  list(APPEND ZUCCHINI_FORMAT_SOURCES
    disassembler_elf.cc
    rel32_finder.cc
    rel32_utils.cc
    reloc_elf.cc
  )
endif()

set(ZUCCHINI_SOURCES)
foreach(source IN LISTS ZUCCHINI_COMMON_SOURCES ZUCCHINI_FORMAT_SOURCES)
  list(APPEND ZUCCHINI_SOURCES "${ZUCCHINI_SOURCE_DIR}/${source}")
endforeach()

add_library(zucchini_objects OBJECT
  ${ZUCCHINI_SOURCES}
  src/zucchini_standalone.cc
)

set_target_properties(zucchini_objects PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

target_include_directories(zucchini_objects
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/compat>"
    "$<BUILD_INTERFACE:${ZUCCHINI_SOURCE_DIR}/aosp/include>"
     "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libsais/include>"
)

target_compile_definitions(zucchini_objects
  PUBLIC
    _FILE_OFFSET_BITS=64
    ZUCCHINI_STANDALONE_ENABLE_DEX=$<BOOL:${ZUCCHINI_ENABLE_DEX}>
    ZUCCHINI_STANDALONE_ENABLE_ELF=$<BOOL:${ZUCCHINI_ENABLE_ELF}>
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(zucchini_objects PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-sign-compare
    "-include${CMAKE_CURRENT_SOURCE_DIR}/compat/zucchini_compat.h"
  )
  if(ZUCCHINI_NATIVE_OPTIMIZE)
    target_compile_options(zucchini_objects PRIVATE)
  endif()
endif()

    option(ZUCCHINI_NO_STRICT_ALIASING "Compile with -fno-strict-aliasing on every platform" OFF)
    if(ZUCCHINI_NO_STRICT_ALIASING OR ANDROID)
      target_compile_options(zucchini_objects PRIVATE -fno-strict-aliasing)
    endif()

function(zucchini_configure_public_target target_name)
  target_include_directories(${target_name} PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/compat>"
    "$<BUILD_INTERFACE:${ZUCCHINI_SOURCE_DIR}/aosp/include>"
    "$<INSTALL_INTERFACE:include>"
  )
  target_compile_definitions(${target_name} PUBLIC
    ZUCCHINI_STANDALONE_ENABLE_DEX=$<BOOL:${ZUCCHINI_ENABLE_DEX}>
    ZUCCHINI_STANDALONE_ENABLE_ELF=$<BOOL:${ZUCCHINI_ENABLE_ELF}>
  )
endfunction()

if(ZUCCHINI_BUILD_STATIC)
  add_library(zucchini_static STATIC $<TARGET_OBJECTS:zucchini_objects>)
  set_target_properties(zucchini_static PROPERTIES OUTPUT_NAME zucchini)
  zucchini_configure_public_target(zucchini_static)
endif()

if(ZUCCHINI_BUILD_SHARED)
  add_library(zucchini_shared SHARED $<TARGET_OBJECTS:zucchini_objects>)
  set_target_properties(zucchini_shared PROPERTIES
    OUTPUT_NAME zucchini
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
  )
  zucchini_configure_public_target(zucchini_shared)
endif()

    option(ZUCCHINI_STATIC_EXE "Link the CLI fully static (for cross-running on other OSes)" OFF)
    if(ZUCCHINI_BUILD_CLI)
      add_executable(zucchini_cli app/zucchini_main.cc)
      set_target_properties(zucchini_cli PROPERTIES OUTPUT_NAME zucchini)
      target_include_directories(zucchini_cli PRIVATE include)
      if(TARGET zucchini_static)
        target_link_libraries(zucchini_cli PRIVATE zucchini_static libsais)
      elseif(TARGET zucchini_shared)
        target_link_libraries(zucchini_cli PRIVATE zucchini_shared libsais)
      else()
        message(FATAL_ERROR "The CLI requires either the static or shared library")
      endif()
      if(ZUCCHINI_STATIC_EXE)
        target_link_options(zucchini_cli PRIVATE -static)
      endif()
    endif()

include(GNUInstallDirs)

if(TARGET zucchini_static)
  install(TARGETS zucchini_static
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if(TARGET zucchini_shared)
  install(TARGETS zucchini_shared
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if(TARGET zucchini_cli)
  install(TARGETS zucchini_cli
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

install(FILES include/zucchini_standalone.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install the upstream buffer-level C++ API and the small compatibility headers.
install(DIRECTORY "${ZUCCHINI_SOURCE_DIR}/aosp/include/components"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h")
install(DIRECTORY compat/base compat/build
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h")
install(FILES compat/components/zucchini/buildflags.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/components/zucchini)
