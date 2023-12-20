cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project(kapua-tracker CXX C ASM)

# load extra modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include_directories (
	${CMAKE_SOURCE_DIR}/src
	${YAML_CPP_INCLUDE_DIR}
)

add_library(
	kapua-tracker SHARED 
	src/Logger.cpp
	src/Tracker.cpp
)

find_package(LIBYAML REQUIRED)
include_directories(${LIBYAML_INCLUDE_DIR})

find_package(Boost 1.83.0 REQUIRED COMPONENTS thread)
include_directories(${Boost_INCLUDE_DIR})

find_package(nlohmann_json 3.9.0 REQUIRED)
include_directories(${nlohmann_json_INCLUDE_DIRS})

target_link_libraries(kapua-tracker LINK_PUBLIC
	${LIBYAML_LIBRARIES}
	${Boost_LIBRARIES}
	nlohmann_json::nlohmann_json
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Examples

project(kapua_link CXX C ASM)

add_executable(
	kapuatrackerd
	src/main.cpp
)
target_link_libraries(kapuatrackerd kapua-tracker)