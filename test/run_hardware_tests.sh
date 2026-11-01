#!/usr/bin/env bash
# ==============================================================================
# test/run_hardware_tests.sh
# Native C++ Runner for Role 4: Cell Hardware & Scalable Matrix HAL Tests
#
# Fully self-contained within the repository.
# Uses test/mocks/include (Arduino.h, ESP32Servo.h, unity.h) for cross-platform
# execution on any machine with clang++ or g++.
# ==============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

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

echo "=========================================================="
echo " Building & Running Role 4 Matrix Grid Tests (Native C++)"
echo " Compiler: ${CXX_BIN}"
echo "=========================================================="

"${CXX_BIN}" -std=c++14 \
    -I "${ROOT_DIR}/test/mocks/include" \
    -I "${ROOT_DIR}/lib/CellHardwareControl/src" \
    -I "${ROOT_DIR}/include" \
    "${ROOT_DIR}/lib/CellHardwareControl/src/CellActuators.cpp" \
    "${ROOT_DIR}/lib/CellHardwareControl/src/CellHardware.cpp" \
    "${ROOT_DIR}/lib/CellHardwareControl/src/CellHardwareController.cpp" \
    "${ROOT_DIR}/test/test_cell_hardware/test_cell_hardware.cpp" \
    -o "${SCRIPT_DIR}/test_hardware_runner"

"${SCRIPT_DIR}/test_hardware_runner"
STATUS=$?
rm -f "${SCRIPT_DIR}/test_hardware_runner"
exit $STATUS
