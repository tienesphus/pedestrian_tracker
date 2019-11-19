
set(NCS_RESET ${CMAKE_INSTALL_PREFIX}/bin/ncs_reset)
set(WEBSTART_SCRIPT_LOC ${CMAKE_INSTALL_PREFIX}/bin/buscountwebstart)

set(BUSCOUNTD_LOCATION ${SOURCE_DIR}/build/bin/buscountd)
set(WEBSTART_LOCATION ${SOURCE_DIR}/build/bin/buscountserver)
set(CHECKMASTER_LOCATION "/usr/local/lib/buscount-wifi/check_master")
set(RAMDISK_LOCATION ${SOURCE_DIR}/ram_disk)

if ("$ENV{INTEL_OPENVINO_DIR}" STREQUAL "")
    message(FATAL_ERROR "OpenVINO must be initialised")
endif()

set(OPENVINO_SETUPVARS "$ENV{INTEL_OPENVINO_DIR}/bin/setupvars.sh" CACHE FILEPATH "Setupvars script")

string(REGEX MATCH "[^/].*" RAMDISK_NAME ${RAMDISK_LOCATION})
string(REGEX REPLACE "/" "-" RAMDISK_NAME ${RAMDISK_NAME})

set(RAMDISK_NAME ${RAMDISK_NAME}.mount)

configure_file(buscountd.service.in buscountd.service)
configure_file(buswebserver.service.in buswebserver.service)
configure_file(buscountwebstart.in buscountwebstart)
configure_file(buscount-ramdisk.mount.in buscount-ramdisk.mount)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/buscountd.service ${CMAKE_CURRENT_BINARY_DIR}/buswebserver.service 
    DESTINATION /etc/systemd/system/
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/buscount-ramdisk.mount
    DESTINATION /etc/systemd/system/
    RENAME ${RAMDISK_NAME}
)

install(
    PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/ncs_reset ${CMAKE_CURRENT_BINARY_DIR}/buscountwebstart
    DESTINATION bin
)
