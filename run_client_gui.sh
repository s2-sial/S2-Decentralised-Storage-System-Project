#!/usr/bin/env bash
cd "$(dirname "$0")"
chmod 700 /run/user/1000 2>/dev/null || true
export LIBGL_ALWAYS_SOFTWARE=1
./build/client_gui
