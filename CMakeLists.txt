if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -std=c++11 -Wall")
endif()

add_executable(pwomatic main.cpp)

target_link_libraries(pwomatic pwgen)
add_library(pwgen STATIC sha256.cpp pwgen.cpp)
