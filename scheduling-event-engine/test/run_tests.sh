#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODULE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "=========================================="
echo " Building Scheduling & Time/Event Engine Unit Tests"
echo "=========================================="

clang++ -std=c++14 -Wall -Wextra \
    -I "${MODULE_DIR}/include" \
    "${MODULE_DIR}/src/schedule_event_engine.cpp" \
    "${MODULE_DIR}/src/time_provider.cpp" \
    "${MODULE_DIR}/src/configuration_adapter.cpp" \
    "${SCRIPT_DIR}/test_engine_unit.cpp" \
    -o "${SCRIPT_DIR}/test_runner"

echo "Executing test suite..."
echo ""
"${SCRIPT_DIR}/test_runner"
STATUS=$?

rm -f "${SCRIPT_DIR}/test_runner"
exit $STATUS
