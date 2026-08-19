#!/bin/bash
# Copyright (c) 2026 Fixstars Corporation
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

if [ $# -lt 1 ]; then
    echo "Usage: $0 <local-binary-path>" >&2
    exit 1
fi

LOCAL_BINARY="${1}"

# === 転送先設定 ===
# REMOTE_USER と REMOTE_HOST は環境変数から取得
TARGET_HOST="${REMOTE_USER}@${REMOTE_HOST}"
TARGET_DIR="/home/${REMOTE_USER}"
REMOTE_BINARY="${TARGET_DIR}/$(basename "${LOCAL_BINARY}")"
BINARY_NAME="$(basename "${LOCAL_BINARY}")"

echo "=== Transfer binary to target ==="
scp "${LOCAL_BINARY}" "${TARGET_HOST}:${REMOTE_BINARY}"

echo "=== Run benchmark on target ==="
ssh "${TARGET_HOST}" "cd ${TARGET_DIR} && ./${BINARY_NAME}"