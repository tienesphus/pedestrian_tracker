

# Added so we can use traditional pkg-config style build information.
find_package(PkgConfig REQUIRED)

# TODO I don't think cmake module path should need to be set
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(TBB QUIET)
if(NOT TBB_FOUND)
	pkg_check_modules(TBB REQUIRED tbb)
	if(NOT TBB_INCLUDE_DIRS)
		set(TBB_INCLUDE_DIRS TBB_INCLUDEDIR)
	endif()
	set(TBB_LIBRARIES_RELEASE TBB_LIBRARIES)
	set(TBB_LIBRARIES_DEBUG TBB_LIBRARIES)
endif()


pkg_check_modules(GSTRTSPSRV REQUIRED gstreamer-rtsp-server-1.0)

# These need to be placed after TBB to be able to make use of them.
find_package(OpenCV 4.0 REQUIRED)
find_package(InferenceEngine 1.5 REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(SQLITE3 REQUIRED sqlite3)

add_library(buscount OBJECT
	libbuscount.cpp
	detection.cpp
	utils.cpp
	optional.hpp
	world.cpp
	detector.cpp
	detector_opencv.cpp
	detector_openvino.cpp
	tracker_component.cpp
	feature_affinity.cpp
	position_affinity.cpp
	event.cpp
)

target_compile_definitions(buscount PUBLIC
    $<$<CONFIG:Debug>:TBB_USE_DEBUG=1>
)

target_include_directories(buscount PUBLIC
	${OpenCV_INCLUDE_DIRS}
	${InferenceEngine_INCLUDE_DIRS}
)

add_library(buscount_static STATIC $<TARGET_OBJECTS:buscount>)

# Might be helpful in the future, but for now we'll just leave this be
#add_library(buscount_shared SHARED $<TARGET_OBJECTS:buscount>)

target_link_libraries(buscount_static INTERFACE
	${TBB_LIBRARIES}
    opencv_core opencv_dnn
    IE::inference_engine
    Threads::Threads
)


add_executable(buscountcli     buscountcli.cpp server/server_client.cpp)
add_executable(buscountd       buscountd.cpp)

target_include_directories(buscountd PRIVATE
	${GSTRTSPSRV_INCLUDE_DIRS}
)

target_compile_definitions(buscountcli PRIVATE SOURCE_DIR="${bus_count_SOURCE_DIR}")

target_link_libraries(buscountcli buscount_static opencv_highgui sqlite3 jsoncpp)
target_link_libraries(buscountd ${GSTRTSPSRV_LIBRARIES})

# Buscount server must be built with different arch type
execute_process(COMMAND uname -m OUTPUT_VARIABLE SYS_ARCH)
string(STRIP "${SYS_ARCH}" SYS_ARCH)
target_compile_options(buscount_static PUBLIC
		$<$<STREQUAL:"${SYS_ARCH}","armv7l">:-march=armv7-a>
)

add_subdirectory(server)