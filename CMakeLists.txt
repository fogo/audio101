cmake_minimum_required(VERSION 3.7)
project(audio101)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_VERBOSE_MAKEFILE OFF)

find_package(Threads)

add_library(pcm_player STATIC src/audio101/pcm_player.cpp)
target_include_directories(pcm_player PUBLIC include/)
target_link_libraries(pcm_player PRIVATE asound ${CMAKE_THREAD_LIBS_INIT})

add_executable(playpcm src/audio101/playpcm.cpp)
target_link_libraries(playpcm PRIVATE pcm_player)

add_library(fakeasound SHARED src/fakeasound/fakeasound.cpp)
target_include_directories(fakeasound PUBLIC include/)

add_subdirectory(test)

enable_testing()
add_test(
    NAME test_bin
    COMMAND playpcm --help
)

add_test(
    NAME test_pcm_player
    COMMAND test_pcm_player
)
set_tests_properties(test_pcm_player PROPERTIES ENVIRONMENT LD_PRELOAD=$<TARGET_FILE:fakeasound>)
