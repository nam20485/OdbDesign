#!/bin/bash

# OdbDesign Linux Setup Script (shim)
# This file remains for backward compatibility. The script has moved to scripts/setup-linux.sh

# Delegate to the new location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

if [[ -f "$PROJECT_ROOT/scripts/setup-linux.sh" ]]; then
  exec "$PROJECT_ROOT/scripts/setup-linux.sh" "$@"
else
  echo "Error: scripts/setup-linux.sh not found."
  exit 1
fi