project (pwomatic)
cmake_minimum_required(VERSION 2.8)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -std=c++11 -Wall")
endif()

# gtkmm
find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-3.0)
include_directories(${GTKMM_INCLUDE_DIRS})

add_library(pwgen STATIC sha256.cpp pwgen.cpp)

add_executable(pwomatic main.cpp)
target_link_libraries(pwomatic pwgen)

add_executable(gpw gui.cpp)
target_link_libraries(gpw pwgen ${GTKMM_LIBRARIES})

