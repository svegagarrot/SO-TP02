#!/usr/bin/env bash
# PVS-Studio analysis automation script
# Usage: ./scripts/pvs.sh [--exclude <dir>]...

set -euo pipefail

# Configuration
PVS_LOG="pvs.log"
PVS_REPORT_DIR="pvs-report"
SUPPRESS_FILE=".pvs/PVS-Studio.suppress"

# Parse exclusion directories
EXCLUDE_ARGS=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        --exclude)
            shift
            if [[ $# -eq 0 ]]; then
                echo "Error: --exclude requires a directory argument" >&2
                exit 1
            fi
            EXCLUDE_ARGS+=("-e" "$1")
            shift
            ;;
        *)
            echo "Error: Unknown argument '$1'" >&2
            echo "Usage: $0 [--exclude <dir>]..." >&2
            exit 1
            ;;
    esac
done

echo "==> Step 1: Tracing build with PVS-Studio..."
pvs-studio-analyzer trace -- make -j

echo ""
echo "==> Step 2: Running static analysis..."
ANALYZE_ARGS=(-o "$PVS_LOG")

# Add exclusions if provided
if [[ ${#EXCLUDE_ARGS[@]} -gt 0 ]]; then
    ANALYZE_ARGS+=("${EXCLUDE_ARGS[@]}")
fi

# Add suppressions file if it exists
if [[ -f "$SUPPRESS_FILE" ]]; then
    echo "    Using suppressions from: $SUPPRESS_FILE"
    ANALYZE_ARGS+=("-S" "$SUPPRESS_FILE")
fi

# For open-source projects: use comment file if PVS_COMMENT env variable is set
if [[ -n "${PVS_COMMENT:-}" ]] && [[ -f "$PVS_COMMENT" ]]; then
    echo "    Using open-source license comment from: $PVS_COMMENT"
    ANALYZE_ARGS+=("--lic-file" "$PVS_COMMENT")
fi

pvs-studio-analyzer analyze "${ANALYZE_ARGS[@]}"

echo ""
echo "==> Step 3: Converting report to HTML..."
plog-converter -a GA:1,2 -t fullhtml -o "$PVS_REPORT_DIR" "$PVS_LOG"

echo ""
echo "==> Analysis complete!"
echo "    Report: $PVS_REPORT_DIR/index.html"

# Check for high-severity diagnostics (GA:1)
echo ""
echo "==> Checking for high-severity issues (GA:1)..."
HIGH_SEVERITY_COUNT=$(plog-converter -a GA:1 -t tasklist "$PVS_LOG" 2>/dev/null | grep -v "^$" | wc -l || true)

if [[ $HIGH_SEVERITY_COUNT -gt 0 ]]; then
    echo "    ❌ Found $HIGH_SEVERITY_COUNT high-severity issue(s)!"
    echo "    Review the report at: $PVS_REPORT_DIR/index.html"
    exit 1
else
    echo "    ✅ No high-severity issues found"
    exit 0
fi
