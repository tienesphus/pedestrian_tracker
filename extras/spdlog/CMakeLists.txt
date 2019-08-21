add_library(spdlog STATIC spdlog.cpp)
target_include_directories(spdlog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(spdlog PRIVATE SPDLOG_COMPILED_LIB)

add_library(spdlog::spdlog ALIAS spdlog)
