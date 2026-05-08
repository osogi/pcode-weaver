#!/usr/bin/env sh
set -eu

if [ "$#" -gt 0 ]; then
  CONFIG_PATH="$1"
else
  if ! command -v reoxide >/dev/null 2>&1; then
    echo "error: reoxide is not available in this environment" >&2
    echo "activate your ReOxide environment or pass current.yaml path manually" >&2
    exit 1
  fi
  CONFIG_PATH="$(reoxide print-plugin-dir)/../current.yaml"
fi

if [ ! -f "$CONFIG_PATH" ]; then
  echo "error: config file not found: $CONFIG_PATH" >&2
  exit 1
fi

if grep -Eq '^[[:space:]]*-[[:space:]]*action:[[:space:]]*pcodeweaver[[:space:]]*$' "$CONFIG_PATH"; then
  echo "pcodeweaver action is already installed in $CONFIG_PATH"
  exit 0
fi

if ! grep -Eq '^[[:space:]]*-[[:space:]]*action:[[:space:]]*stop[[:space:]]*$' "$CONFIG_PATH"; then
  echo "error: could not find '- action: stop' in $CONFIG_PATH" >&2
  exit 1
fi

TMP_FILE="${CONFIG_PATH}.tmp"
BACKUP_FILE="${CONFIG_PATH}.bak"

awk '
  /^[[:space:]]*-[[:space:]]*action:[[:space:]]*stop[[:space:]]*$/ && !inserted {
    print "- action: pcodeweaver"
    print "  group: analysis"
    inserted = 1
  }
  { print }
' "$CONFIG_PATH" > "$TMP_FILE"

cp "$CONFIG_PATH" "$BACKUP_FILE"
mv "$TMP_FILE" "$CONFIG_PATH"

echo "installed pcodeweaver action in $CONFIG_PATH"
echo "backup written to $BACKUP_FILE"
