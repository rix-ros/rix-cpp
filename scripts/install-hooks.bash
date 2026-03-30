#!/usr/bin/env bash
# Installs the project's git hooks into .git/hooks/.
#
# Usage:  bash scripts/install-hooks.bash
#
# Re-running this script is safe — it overwrites existing hooks.

set -euo pipefail

REPO_ROOT=$(git rev-parse --show-toplevel 2>/dev/null) || {
  echo "ERROR: Must be run from inside the git repository." >&2
  exit 1
}

HOOKS_DIR="$REPO_ROOT/.git/hooks"
SCRIPTS_DIR="$REPO_ROOT/scripts"

install_hook() {
  local hook_name="$1"
  local src="$SCRIPTS_DIR/$hook_name"
  local dst="$HOOKS_DIR/$hook_name"

  if [[ ! -f "$src" ]]; then
    echo "WARNING: $src not found, skipping." >&2
    return
  fi

  cp "$src" "$dst"
  chmod +x "$dst"
  echo "Installed: .git/hooks/$hook_name"
}

install_hook pre-commit

echo ""
echo "Git hooks installed successfully."
echo "To uninstall, delete the files under .git/hooks/."
