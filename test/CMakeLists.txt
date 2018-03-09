SET(COVERAGE OFF CACHE BOOL "Coverage")

add_library(test_meta INTERFACE)
target_include_directories(test_meta INTERFACE ../include)
target_link_libraries(test_meta INTERFACE dl)
if (COVERAGE)
    target_compile_options(test_meta INTERFACE --coverage)
    target_link_libraries(test_meta INTERFACE --coverage)
endif()

add_executable(test_pcm_player test_pcm_player.cpp)
target_link_libraries(test_pcm_player PRIVATE test_meta pcm_player)
