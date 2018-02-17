cmake_minimum_required(VERSION 3.7)
project(audio101)

set(CMAKE_CXX_STANDARD 14)

find_package(Threads)

set(PCM_PLAYER_SRC_FILES src/playpcm.cpp src/pcm_player.cpp)
add_executable(playpcm ${PCM_PLAYER_SRC_FILES})
target_link_libraries(playpcm PUBLIC asound ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(playpcm PRIVATE include/)
