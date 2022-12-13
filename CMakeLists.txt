cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Chatroom VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")

configure_file(src/ChatroomConfig.h.in ChatroomConfig.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

include_directories(lib/util/src)

file(GLOB server_SRC 
    "src/server/*.cpp"
    "src/server/*.hpp"
)
file(GLOB client_SRC 
    "src/client/*.cpp"
    "src/client/*.hpp"
)

add_executable(server ${server_SRC})
add_executable(client ${client_SRC})
