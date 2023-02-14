cmake_minimum_required(VERSION 3.14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project("Game Lobby" LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/${CMAKE_GENERATOR_PLATFORM}/${CMAKE_BUILD_TYPE})

set(CLIENT_NAME "Client")
set(GAME_NAME "Game")
set(LOBBY_NAME "Lobby")
add_executable(${CLIENT_NAME} client.cpp)
add_executable(${LOBBY_NAME} lobby.cpp)
add_executable(${GAME_NAME} game.cpp)

set(ENET_DIR ${CMAKE_SOURCE_DIR}/../3rdParty/enet)
add_subdirectory(${ENET_DIR} 3rdParty/enet)
include_directories(${ENET_DIR}/include)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

target_link_libraries(${CLIENT_NAME} PRIVATE enet)
target_link_libraries(${GAME_NAME} PRIVATE enet)
target_link_libraries(${LOBBY_NAME} PRIVATE enet)
