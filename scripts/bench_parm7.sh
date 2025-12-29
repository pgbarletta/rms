#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <bench_binary> [parm7_path] [iterations]" >&2
  exit 1
fi

bench_bin="$1"
parm7_path="${2:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/daux/binder_wcn.parm7}"
iterations="${3:-5}"

"${bench_bin}" "${parm7_path}" "${iterations}"
