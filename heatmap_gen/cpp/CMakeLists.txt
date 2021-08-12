# Copyright (C) 2018-2019 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
#

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp *.c)
file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp *.h)

add_project(NAME heatmap_gen
    SOURCES ${SOURCES}
    HEADERS ${HEADERS}
    INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
    DEPENDENCIES monitors)
