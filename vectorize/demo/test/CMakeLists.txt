# Copyright (c) 2026 Fixstars Corporation
# SPDX-License-Identifier: Apache-2.0

# Benchmark executable
add_executable(bench main.cpp)
target_link_libraries(bench PRIVATE pose_alignment)