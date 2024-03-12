cmake_minimum_required(VERSION 3.24)

# Set C++ compiler flags before `project`
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set the project name.
set(PROJECT_NAME "Converter")
project(${PROJECT_NAME} LANGUAGES C CXX CUDA)

# Set output directories.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")

# Include cmake scripts
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpm.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies.cmake")

# Set the include directories.
list(APPEND CONVERTER_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/libs
    ${CMAKE_CURRENT_SOURCE_DIR}/modules
    ${tinyply_SOURCE_DIR}/source
)

# Set the linking libraries.
list(APPEND CONVERTER_LIBRARIES
    tinyply
)

link_directories("${CMAKE_CURRENT_SOURCE_DIR}/libs/laszip")

# Set the source files.
aux_source_directory(src SRC_FILES)
aux_source_directory(modules/LasLoader SRC_FILES)
aux_source_directory(modules/tools SRC_FILES)
aux_source_directory(modules/unsuck SRC_FILES)

add_executable(${PROJECT_NAME} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CONVERTER_INCLUDES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${CONVERTER_LIBRARIES} laszip3 ${PCL_LIBRARIES})

