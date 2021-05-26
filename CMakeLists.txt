cmake_minimum_required(VERSION 3.9.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(GCC_COVERAGE_COMPILE_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")

project(PasswordCracker)


include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
)

add_subdirectory(fmt)


file(GLOB all_SRCS
        "${PROJECT_SOURCE_DIR}/include/*.h"
		"${PROJECT_SOURCE_DIR}/include/*.hpp"
        "${PROJECT_SOURCE_DIR}/src/*.cpp"
		"${PROJECT_SOURCE_DIR}/src/*.c"
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)



add_executable(PasswordCracker ${all_SRCS})
target_link_libraries(PasswordCracker PUBLIC crypt fmt::fmt-header-only)


find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(PasswordCracker PUBLIC OpenMP::OpenMP_CXX )
endif()

# MPI
find_package(MPI REQUIRED)
target_link_libraries(PasswordCracker PUBLIC MPI::MPI_CXX)

