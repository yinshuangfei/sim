# SPDX-License-Identifier: BSD-3-Clause
#
# - Find bison executable and provides macros to generate custom build rules
# The module defines the following variables:
#
#  BISON_EXECUTABLE - path to the bison program
#  BISON_VERSION - version of bison
#  BISON_FOUND - true if the program was found
#
# The minimum required version of bison can be specified using the
# standard CMake syntax, e.g. find_package(BISON 2.1.3)
#
# If bison is found, the module defines the macros:
#  BISON_TARGET(<Name> <YaccInput> <CodeOutput> [VERBOSE <file>]
#              [COMPILE_FLAGS <string>])
# which will create  a custom rule to generate  a parser. <YaccInput> is
# the path to  a yacc file. <CodeOutput> is the name  of the source file
# generated by bison.  A header file is also  be generated, and contains
# the  token  list.  If  COMPILE_FLAGS  option is  specified,  the  next
# parameter is  added in the bison  command line.  if  VERBOSE option is
# specified, <file> is created  and contains verbose descriptions of the
# grammar and parser. The macro defines a set of variables:
#  BISON_${Name}_DEFINED - true is the macro ran successfully
#  BISON_${Name}_INPUT - The input source file, an alias for <YaccInput>
#  BISON_${Name}_OUTPUT_SOURCE - The source file generated by bison
#  BISON_${Name}_OUTPUT_HEADER - The header file generated by bison
#  BISON_${Name}_OUTPUTS - The sources files generated by bison
#  BISON_${Name}_COMPILE_FLAGS - Options used in the bison command line
#
#  ====================================================================
#  Example:
#
#   find_package(BISON)
#   BISON_TARGET(MyParser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp)
#   add_executable(Foo main.cpp ${BISON_MyParser_OUTPUTS})
#  ====================================================================

#=============================================================================
# Copyright 2009 Kitware, Inc.
# Copyright 2006 Tristan Carel
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

find_program(BISON_EXECUTABLE NAMES bison win_bison DOC "path to the bison executable")
mark_as_advanced(BISON_EXECUTABLE)

if(BISON_EXECUTABLE)
  # the bison commands should be executed with the C locale, otherwise
  # the message (which are parsed) may be translated
  set(_Bison_SAVED_LC_ALL "$ENV{LC_ALL}")
  set(ENV{LC_ALL} C)

  execute_process(COMMAND ${BISON_EXECUTABLE} --version
    OUTPUT_VARIABLE BISON_version_output
    ERROR_VARIABLE BISON_version_error
    RESULT_VARIABLE BISON_version_result
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  set(ENV{LC_ALL} ${_Bison_SAVED_LC_ALL})

  if(NOT ${BISON_version_result} EQUAL 0)
    message(SEND_ERROR "Command \"${BISON_EXECUTABLE} --version\" failed with output:\n${BISON_version_error}")
  else()
    # Bison++
    if("${BISON_version_output}" MATCHES "^bison\\+\\+")
      string(REGEX REPLACE "^bison\\+\\+ Version ([^,]+).*" "\\1"
        BISON_VERSION "${BISON_version_output}")
    # GNU Bison
    elseif("${BISON_version_output}" MATCHES "^bison[^+]")
      string(REGEX REPLACE "^bison \\(GNU Bison\\) ([^\n]+)\n.*" "\\1"
        BISON_VERSION "${BISON_version_output}")
    elseif("${BISON_version_output}" MATCHES "^GNU Bison ")
      string(REGEX REPLACE "^GNU Bison (version )?([^\n]+).*" "\\2"
        BISON_VERSION "${BISON_version_output}")
    endif()
  endif()

  # internal macro
  macro(BISON_TARGET_option_verbose Name BisonOutput filename)
    list(APPEND BISON_TARGET_cmdopt "--verbose")
    get_filename_component(BISON_TARGET_output_path "${BisonOutput}" PATH)
    get_filename_component(BISON_TARGET_output_name "${BisonOutput}" NAME_WE)
    add_custom_command(OUTPUT ${filename}
      COMMAND ${CMAKE_COMMAND}
      ARGS -E copy
      "${BISON_TARGET_output_path}/${BISON_TARGET_output_name}.output"
      "${filename}"
      DEPENDS
      "${BISON_TARGET_output_path}/${BISON_TARGET_output_name}.output"
      COMMENT "[BISON][${Name}] Copying bison verbose table to ${filename}"
      WORKING_DIRECTORY ${GANESHA_TOP_CMAKE_DIR})
    set(BISON_${Name}_VERBOSE_FILE ${filename})
    list(APPEND BISON_TARGET_extraoutputs
      "${BISON_TARGET_output_path}/${BISON_TARGET_output_name}.output")
  endmacro()

  # internal macro
  macro(BISON_TARGET_option_extraopts Options)
    set(BISON_TARGET_extraopts "${Options}")
    separate_arguments(BISON_TARGET_extraopts)
    list(APPEND BISON_TARGET_cmdopt ${BISON_TARGET_extraopts})
  endmacro()

  #============================================================
  # BISON_TARGET (public macro)
  #============================================================
  #
  macro(BISON_TARGET Name BisonInput BisonOutput)
    set(BISON_TARGET_output_header "")
    set(BISON_TARGET_cmdopt "")
    set(BISON_TARGET_outputs "${BisonOutput}")
    if(NOT ${ARGC} EQUAL 3 AND NOT ${ARGC} EQUAL 5 AND NOT ${ARGC} EQUAL 7)
      message(SEND_ERROR "Usage")
    else()
      # Parsing parameters
      if(${ARGC} GREATER 5 OR ${ARGC} EQUAL 5)
        if("${ARGV3}" STREQUAL "VERBOSE")
          BISON_TARGET_option_verbose(${Name} ${BisonOutput} "${ARGV4}")
        endif()
        if("${ARGV3}" STREQUAL "COMPILE_FLAGS")
          BISON_TARGET_option_extraopts("${ARGV4}")
        endif()
      endif()

      if(${ARGC} EQUAL 7)
        if("${ARGV5}" STREQUAL "VERBOSE")
          BISON_TARGET_option_verbose(${Name} ${BisonOutput} "${ARGV6}")
        endif()

        if("${ARGV5}" STREQUAL "COMPILE_FLAGS")
          BISON_TARGET_option_extraopts("${ARGV6}")
        endif()
      endif()

      # Header's name generated by bison (see option -d)
      list(APPEND BISON_TARGET_cmdopt "-d")
      string(REGEX REPLACE "^(.*)(\\.[^.]*)$" "\\2" _fileext "${ARGV2}")
      string(REPLACE "c" "h" _fileext ${_fileext})
      string(REGEX REPLACE "^(.*)(\\.[^.]*)$" "\\1${_fileext}"
          BISON_${Name}_OUTPUT_HEADER "${ARGV2}")
      list(APPEND BISON_TARGET_outputs "${BISON_${Name}_OUTPUT_HEADER}")

      add_custom_command(OUTPUT ${BISON_TARGET_outputs}
        ${BISON_TARGET_extraoutputs}
        COMMAND ${BISON_EXECUTABLE}
        ARGS ${BISON_TARGET_cmdopt} -o ${ARGV2} ${ARGV1}
        DEPENDS ${ARGV1}
        COMMENT "[BISON][${Name}] Building parser with bison ${BISON_VERSION}"
        WORKING_DIRECTORY ${GANESHA_TOP_CMAKE_DIR})

      # define target variables
      set(BISON_${Name}_DEFINED TRUE)
      set(BISON_${Name}_INPUT ${ARGV1})
      set(BISON_${Name}_OUTPUTS ${BISON_TARGET_outputs})
      set(BISON_${Name}_COMPILE_FLAGS ${BISON_TARGET_cmdopt})
      set(BISON_${Name}_OUTPUT_SOURCE "${BisonOutput}")

    endif()
  endmacro()
  #
  #============================================================

endif()

include(${CMAKE_CURRENT_LIST_DIR}/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(BISON REQUIRED_VARS  BISON_EXECUTABLE
                                        VERSION_VAR BISON_VERSION)

# FindBISON.cmake ends here
