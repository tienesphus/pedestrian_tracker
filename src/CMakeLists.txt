

# Added so we can use traditional pkg-config style build information.
find_package(PkgConfig REQUIRED)

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


add_executable(buscountcli     buscountcli.cpp)
add_executable(buscountd       buscountd.cpp)

target_include_directories(buscountd PRIVATE
	${GSTRTSPSRV_INCLUDE_DIRS}
)

target_compile_definitions(buscountcli PRIVATE SOURCE_DIR="${bus_count_SOURCE_DIR}")

target_link_libraries(buscountcli buscount_static server_static opencv_highgui)
target_link_libraries(buscountd ${GSTRTSPSRV_LIBRARIES})

add_subdirectory(server)