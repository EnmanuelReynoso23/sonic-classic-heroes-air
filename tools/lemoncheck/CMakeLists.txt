cmake_minimum_required(VERSION 3.16)
project(lemoncheck CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# Path to a checkout of https://github.com/Eukaryot/sonic3air
set(S3AIR_REPO "" CACHE PATH "Path to the sonic3air repository")

set(RMX_SOURCE ${S3AIR_REPO}/librmx/source)
set(LEMON_SOURCE ${S3AIR_REPO}/Oxygen/lemonscript/source)

file(GLOB_RECURSE RMX_FILES ${RMX_SOURCE}/rmxbase/*.cpp)
file(GLOB_RECURSE LEMON_FILES ${LEMON_SOURCE}/lemon/*.cpp)

add_executable(lemoncheck main.cpp ${RMX_FILES} ${LEMON_FILES})
target_include_directories(lemoncheck PRIVATE ${RMX_SOURCE} ${LEMON_SOURCE})
target_compile_definitions(lemoncheck PRIVATE LSB_FIRST MAXPATHLEN=256 RMX_LIB)
target_compile_options(lemoncheck PRIVATE -w)
find_package(ZLIB REQUIRED)
target_link_libraries(lemoncheck PRIVATE ZLIB::ZLIB pthread)
