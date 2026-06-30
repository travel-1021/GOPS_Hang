cmake_minimum_required(VERSION 3.20)

# Possible cross-compilation?
# set(EXTENSION_PYTHON_VERSION "")

set(EXTENSION_NAME rl_water)
set(EXTENSION_VERSION "11.18")
set(EXTENSION_AUTHOR "The MathWorks, Inc.")

set(EXTENSION_SOURCES
    module.cc
    model/rl_water.cpp
    model/rl_water_data.cpp
    model/rtGetInf.cpp
    model/rt_nonfinite.cpp
    
)
set(EXTENSION_INCLUDE
    include/
    model/
)
set(EXTENSION_HEADERS
    raw_env.h
    common_env.h
    gym_env.h
    
)
project(${EXTENSION_NAME})

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ version selection")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

execute_process(COMMAND "pybind11-config" "--cmakedir" RESULT_VARIABLE PROCESS_RESULT OUTPUT_VARIABLE PROCESS_OUTPUT ERROR_VARIABLE PROCESS_ERROR COMMAND_ERROR_IS_FATAL ANY OUTPUT_STRIP_TRAILING_WHITESPACE)
if (PROCESS_RESULT EQUAL 0)
    message(STATUS "Find pybind11 cmake path: ${PROCESS_OUTPUT}")
    set(pybind11_DIR "${PROCESS_OUTPUT}")
else()
    message(FATAL "Failed to find pybind11 with error: ${PROCESS_ERROR}")
endif()

set(Python_FIND_VIRTUALENV FIRST)
set(Python_FIND_REGISTRY LAST)
set(Python_FIND_FRAMEWORK LAST)
if (DEFINED EXTENSION_PYTHON_VERSION AND NOT EXTENSION_PYTHON_VERSION STREQUAL "")
    find_package(Python ${EXTENSION_PYTHON_VERSION} EXACT COMPONENTS Interpreter REQUIRED)
    find_package(Python ${EXTENSION_PYTHON_VERSION} EXACT COMPONENTS Development REQUIRED)
else()
    find_package(Python COMPONENTS Interpreter REQUIRED)
    find_package(Python COMPONENTS Development REQUIRED)
endif()
find_package(pybind11 REQUIRED)

pybind11_add_module(${EXTENSION_NAME} ${EXTENSION_SOURCES} ${EXTENSION_HEADERS})

target_compile_definitions(${EXTENSION_NAME} PRIVATE FMT_HEADER_ONLY)
target_compile_definitions(${EXTENSION_NAME} PRIVATE PORTABLE_WORDSIZES)
target_compile_definitions(${EXTENSION_NAME} PRIVATE SLXPY_EXTENSION_NAME=${EXTENSION_NAME})
if(DEFINED EXTENSION_VERSION)
    target_compile_definitions(${EXTENSION_NAME} PRIVATE SLXPY_EXTENSION_VERSION=${EXTENSION_VERSION})
endif()
if(DEFINED EXTENSION_AUTHOR)
    target_compile_definitions(${EXTENSION_NAME} PRIVATE SLXPY_EXTENSION_AUTHOR=${EXTENSION_AUTHOR})
endif()
if(DEFINED EXTENSION_LICENSE)
    target_compile_definitions(${EXTENSION_NAME} PRIVATE SLXPY_EXTENSION_LICENSE=${EXTENSION_LICENSE})
endif()

target_include_directories(${EXTENSION_NAME} PRIVATE ${EXTENSION_INCLUDE})
set_target_properties(${EXTENSION_NAME} PROPERTIES VS_DEBUGGER_COMMAND "${Python_EXECUTABLE}")
set_target_properties(${EXTENSION_NAME} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "test_extension.py")
set_target_properties(${EXTENSION_NAME} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PYTHONPATH=${PROJECT_BINARY_DIR}/$<CONFIG>")
set_target_properties(${EXTENSION_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
