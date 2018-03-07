cmake_minimum_required(VERSION 3.7)
project(audio101)

set(CMAKE_CXX_STANDARD 14)

find_package(Threads)

set(PCM_PLAYER_SRC_FILES src/audio101/playpcm.cpp src/audio101/pcm_player.cpp)
add_executable(playpcm ${PCM_PLAYER_SRC_FILES})
target_link_libraries(playpcm PUBLIC asound ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(playpcm PRIVATE include/)

# Still have to come w/ a proper test. Testing audio is not easy, you know.
# First attempts w/ null device were not promising, maybe use LD_PRELOAD
# w/ fake ALSA lib?
enable_testing()
add_test(
    NAME test_bin
    COMMAND playpcm --help
)
