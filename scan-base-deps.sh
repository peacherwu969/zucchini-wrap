#!/usr/bin/env bash
set -euo pipefail

src=${1:?Usage: $0 /path/to/aosp/external/zucchini}

grep -RhoE '#include "(base|build)/[^"]+"' \
  "$src"/*.cc "$src"/aosp/include/components/zucchini/*.h 2>/dev/null \
  | sort -u
