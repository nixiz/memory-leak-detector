# CMakeList.txt : CMake project for MemControllerService, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("MemControllerService")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost REQUIRED)
find_path(BOOST_INCLUDE_DIR boost)

add_definitions(-DBOOST_STACKTRACE_USE_ADDR2LINE)

message(" Includes - ${BOOST_INCLUDE_DIR}")
include_directories( ${BOOST_INCLUDE_DIR} )

link_directories(${Boost_LIBRARY_DIR})


set(header_files "malloc_allocator.hpp" "new.hpp" "MemoryEntry.h" "stack_helper.h" "MemLeakControllerService.h" "MemControllerAgent.h")
set(source_files "MemLeakControllerService.cpp" "MemControllerAgent.cpp")
# Add source to this project's executable.
add_executable (${PROJECT_NAME} "main.cpp" ${source_files} ${header_files})

target_link_libraries(${PROJECT_NAME} ${BOOST_LIBRARIES} )

# TODO: Add tests and install targets if needed.
