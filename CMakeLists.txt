cmake_minimum_required(VERSION 3.13)

set(CMAKE_VERBOSE_MAKEFILE OFF)

project(cpp_capstone LANGUAGES CXX)

include(CTest)

# set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# place binaries and libraries according to GNU standards
include(GNUInstallDirs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

include(FetchContent)
FetchContent_Declare(gtest
  QUIET
  URL https://github.com/google/googletest/archive/release-1.11.0.tar.gz
)

# configure build of googletest
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(gtest)

enable_testing()

find_package(GTest)

add_executable(test_core ${CMAKE_CURRENT_SOURCE_DIR}/test/test_complete.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/test/classdefs.hpp)

target_include_directories(test_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hpp/)

target_link_libraries(test_core PRIVATE gtest_main)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG_PRINTOUT")

gtest_discover_tests(test_core
  PROPERTIES
    LABELS "unit"
  DISCOVERY_TIMEOUT  # how long to wait (in seconds) before crashing
    240
  )

find_package(OpenCV 4.1 REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})
link_directories(${OpenCV_LIBRARY_DIRS})
add_definitions(${OpenCV_DEFINITIONS})


# Build singlethreaded opencv solutioon
add_executable(cascade_classifier_singlethread 
              ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/cascade_classifier_singlethread.cpp)
target_link_libraries( cascade_classifier_singlethread ${OpenCV_LIBRARIES})

  
# Build multithreaded opencv solutioon
add_executable(cascade_classifier_multithread 
              ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/cascade_classifier_multithread.cpp)
target_include_directories(cascade_classifier_multithread PUBLIC 
              ${CMAKE_CURRENT_SOURCE_DIR}/src/hpp/)
target_link_libraries( cascade_classifier_multithread ${OpenCV_LIBRARIES})

  
    



