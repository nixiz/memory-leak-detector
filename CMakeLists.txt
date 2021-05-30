# CMakeList.txt : CMake project for MemControllerService, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("MemControllerService")

find_package(BOOST REQUIRED)

include_directories( ${Boost_INCLUDE_DIR} )

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add source to this project's executable.
add_executable (MemControllerService "MemControllerService.cpp" "MemControllerService.h" "new.hpp" "MemControllerAgent.h" "MemControllerAgent.cpp")

#target_link_libraries( MemControllerService LINK_PUBLIC ${Boost_LIBRARIES} )

# TODO: Add tests and install targets if needed.
