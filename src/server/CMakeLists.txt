add_executable(buscountserver  buscountserver.cpp)

target_link_libraries(buscountserver drogon trantor uuid pthread jsoncpp dl z)

configure_file(index.html index.html COPYONLY)