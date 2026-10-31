#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
exec "${PYTHON:-python3}" "${ROOT}/psxrecomp/tools/package_title_release.py" \
    package --root "${ROOT}" "$@"
