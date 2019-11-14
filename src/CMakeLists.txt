set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_executable(sanity sanity.cpp)
add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(buscount)
