#!/usr/bin/env bash
# Generate the Doxygen API reference into docs/api/ (gitignored).
# Same entry point for CI (.github/workflows/jekyll-gh-pages.yml) and local use.
set -euo pipefail
# Two statements, deliberately NOT "x="$(...)" && cd": under errexit a
# failing command substitution inside a && list (non-final position) does
# NOT abort the script - it would continue in the wrong cwd and die at
# doxygen's "could not open configuration file" instead. As a bare
# statement the failed assignment aborts here with git's own message.
top="$(git rev-parse --show-toplevel)"; cd "$top"   # doxygen.conf paths are cwd-relative
command -v doxygen >/dev/null || { echo "error: doxygen not installed" >&2; exit 1; }
command -v dot      >/dev/null || { echo "error: graphviz 'dot' not installed" >&2; exit 1; }
echo "doxygen $(doxygen --version), $(dot -V 2>&1)"
doxygen ./OdbDesignLib/doxygen.conf
