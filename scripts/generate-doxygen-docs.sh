#!/usr/bin/env bash
# Generate the Doxygen API reference into docs/api/ (gitignored).
# Same entry point for CI (.github/workflows/jekyll-gh-pages.yml) and local use.
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"   # doxygen.conf paths are cwd-relative
command -v doxygen >/dev/null || { echo "error: doxygen not installed" >&2; exit 1; }
command -v dot      >/dev/null || { echo "error: graphviz 'dot' not installed" >&2; exit 1; }
echo "doxygen $(doxygen --version), $(dot -V 2>&1)"
doxygen ./OdbDesignLib/doxygen.conf
