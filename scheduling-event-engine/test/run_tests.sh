#!/usr/bin/env bash
# ==============================================================================
# scheduling-event-engine/test/run_tests.sh
# Standalone Unit Test Runner for Role 2: Scheduling & Time/Event Engine
#
# Detects available C++ compiler (CXX, clang++, or g++) for portability.
# ==============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODULE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Detect available C++ compiler (supports CXX env, clang++, or g++)
CXX_BIN="${CXX}"
if [ -z "${CXX_BIN}" ]; then
    if command -v clang++ &> /dev/null; then
        CXX_BIN="clang++"
    elif command -v g++ &> /dev/null; then
        CXX_BIN="g++"
    else
        echo "ERROR: Neither clang++ nor g++ was found in PATH. Please install a C++ compiler."
        exit 1
    fi
fi

echo "=========================================="
echo " Building Scheduling & Time/Event Engine Unit Tests"
echo " Compiler: ${CXX_BIN}"
echo "=========================================="

"${CXX_BIN}" -std=c++14 -Wall -Wextra \
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
