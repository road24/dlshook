cmake_minimum_required(VERSION 3.10)
project(dlshook)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -fPIC")

# Create the shared library
add_library(dlshook SHARED 
    #${CMAKE_CURRENT_SOURCE_DIR}/src/dlshook.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/card_emulator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dog_server.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils.cc
)

# Set include directories
target_include_directories(dlshook PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)


# set the output file name
set_target_properties(dlshook PROPERTIES 
                        OUTPUT_NAME "dlshook"
                        PREFIX ""
)
