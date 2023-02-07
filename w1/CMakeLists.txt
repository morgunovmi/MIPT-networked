cmake_minimum_required(VERSION 3.14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project("Client_Server" LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/${CMAKE_GENERATOR_PLATFORM}/${CMAKE_BUILD_TYPE})

set(CLIENT_NAME "Client")
set(SERVER_NAME "Server")
add_executable(${CLIENT_NAME} socket_tools.cpp client.cpp)
add_executable(${SERVER_NAME} socket_tools.cpp server.cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
target_compile_features(${CLIENT_NAME} PUBLIC cxx_std_20)
target_compile_features(${SERVER_NAME} PUBLIC cxx_std_20)