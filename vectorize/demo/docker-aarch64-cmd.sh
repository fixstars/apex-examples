#!/bin/bash
# Copyright (c) 2026 Fixstars Corporation
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

IMAGE="nvcr.io/drive/driveos-sdk/drive-agx-orin-linux-aarch64-sdk-build-x86:latest"
USER_ID=$(id -u)
GROUP_ID=$(id -g)

echo "=== aarch64 cross build via Docker ==="
echo "Image : ${IMAGE}"

# イメージがローカルにない場合はエラー
if ! docker image inspect "${IMAGE}" > /dev/null 2>&1; then
    echo "Error: Docker image '${IMAGE}' is not found locally." >&2
    exit 1
fi

if [ $# -eq 0 ]; then
    echo "Usage: $0 <command> [args...]" >&2
    exit 1
fi

CMD="$*"

docker run --rm \
    -v "$(pwd):/workspace" \
    -w /workspace \
    --user "${USER_ID}:${GROUP_ID}" \
    --entrypoint /bin/bash \
    "${IMAGE}" \
    -c "${CMD}"

echo "=== Build completed ==="