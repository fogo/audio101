add_library(test_meta INTERFACE)
target_include_directories(test_meta INTERFACE ../include)
target_link_libraries(test_meta INTERFACE dl)

add_executable(test_pcm_player test_pcm_player.cpp)
target_link_libraries(test_pcm_player PRIVATE test_meta pcm_player)
