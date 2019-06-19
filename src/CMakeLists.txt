# For using traditional pkgconfig modules
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

pkg_check_modules(GStreamerRTSPsrv REQUIRED gstreamer-rtsp-server-1.0)
pkg_check_modules(GStreamerMM REQUIRED gstreamermm-1.0)

# These need to be placed after TBB to be able to make use of them.
find_package(OpenCV 4.0 REQUIRED)
find_package(InferenceEngine 1.5 REQUIRED)
find_package(Threads REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

### libbuscount object files.
  # Compiling as a library speeds up the compile process as we're only compiling things once.
add_library(buscount_static STATIC
	libbuscount.cpp
	detection.cpp
	tracker.cpp
	utils.cpp
	optional.hpp
	world.cpp
	detector.cpp
	detector_opencv.cpp
	detector_openvino.cpp
)

set_property(TARGET buscount_static PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(buscount_static PUBLIC
    $<$<CONFIG:Debug>:TBB_USE_DEBUG=1>
)
target_link_libraries(buscount_static PUBLIC
	${TBB_LIBRARIES}
    opencv_core opencv_dnn
    IE::inference_engine
    Threads::Threads
)

### libgstbuscountplugin.
  # Compiling this as a library allows us to create a .so file, which can be loaded by
  # gst-inspect-1.0 and gst-launch-1.0 for debugging purposes.
add_library(gstbuscountplugin_obj_if INTERFACE IMPORTED)
set_property(TARGET gstbuscountplugin_obj_if PROPERTY INTERFACE_LINK_LIBRARIES
    ${GStreamerRTSPsrv_LIBRARIES}
    ${GStreamerMM_LIBRARIES}
)
set_property(TARGET gstbuscountplugin_obj_if PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    $<TARGET_PROPERTY:buscount_static,INCLUDE_DIRECTORIES>
    ${GStreamerRTSPsrv_INCLUDE_DIRS}
    ${GStreamerMM_INCLUDE_DIRS}
)

add_library(gstbuscountplugin_obj OBJECT gstbuscountfilter.cpp)
set_property(TARGET gstbuscountplugin_obj PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(gstbuscountplugin_obj PUBLIC
    $<TARGET_PROPERTY:gstbuscountplugin_obj_if,INTERFACE_INCLUDE_DIRECTORIES>
)


# Static library is for linking directly with gstreamer. Dynamic library is only for
# debugging with gst-launch-1.0 and gst-inspect-1.0
add_library(gstbuscountplugin_static STATIC $<TARGET_OBJECTS:gstbuscountplugin_obj>)
target_link_libraries(gstbuscountplugin_static
    PUBLIC gstbuscountplugin_obj_if
    PRIVATE buscount_static
)
add_library(gstbuscountplugin MODULE $<TARGET_OBJECTS:gstbuscountplugin_obj>)
target_link_libraries(gstbuscountplugin
    PUBLIC gstbuscountplugin_obj_if
    PRIVATE buscount_static
)

add_executable(buscountcli buscountcli.cpp)
add_executable(buscountd   buscountd.cpp)

target_link_libraries(buscountcli buscount_static opencv_highgui)
target_link_libraries(buscountd gstbuscountplugin_static)
