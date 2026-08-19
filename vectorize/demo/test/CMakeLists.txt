# Copyright (c) 2026 Fixstars Corporation
# SPDX-License-Identifier: MIT

# Benchmark executable
add_executable(bench main.cpp)
target_link_libraries(bench PRIVATE pose_alignment)