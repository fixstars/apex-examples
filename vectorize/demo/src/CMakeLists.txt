# Copyright (c) 2026 Fixstars Corporation
# SPDX-License-Identifier: MIT

add_library(pose_alignment STATIC mathlib.cpp evaluate_pose_alignment.cpp)
target_include_directories(pose_alignment PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})