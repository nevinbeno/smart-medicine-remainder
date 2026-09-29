#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "=========================================================="
echo " Building & Running Role 4 Matrix Grid Tests (Native C++)"
echo "=========================================================="

clang++ -std=c++14 \
    -I "${SCRIPT_DIR}/../scratch_mock/include" \
    -I "${ROOT_DIR}/lib/CellHardwareControl/src" \
    "${ROOT_DIR}/lib/CellHardwareControl/src/CellActuators.cpp" \
    "${ROOT_DIR}/lib/CellHardwareControl/src/CellHardware.cpp" \
    "${ROOT_DIR}/lib/CellHardwareControl/src/CellHardwareController.cpp" \
    "${ROOT_DIR}/scratch/test_dynamic_cells_native.cpp" \
    -o "${SCRIPT_DIR}/test_hardware_runner" 2>/dev/null || {
    # Fallback to local scratch include if scratch_mock not present
    clang++ -std=c++14 \
        -I "/Users/gabriel/.gemini/antigravity-cli/brain/5e833f54-1f69-48db-bd8f-f5b89ecf56f4/scratch/include" \
        -I "${ROOT_DIR}/lib/CellHardwareControl/src" \
        "${ROOT_DIR}/lib/CellHardwareControl/src/CellActuators.cpp" \
        "${ROOT_DIR}/lib/CellHardwareControl/src/CellHardware.cpp" \
        "${ROOT_DIR}/lib/CellHardwareControl/src/CellHardwareController.cpp" \
        "/Users/gabriel/.gemini/antigravity-cli/brain/5e833f54-1f69-48db-bd8f-f5b89ecf56f4/scratch/test_dynamic_cells_native.cpp" \
        -o "${SCRIPT_DIR}/test_hardware_runner"
}

"${SCRIPT_DIR}/test_hardware_runner"
STATUS=$?
rm -f "${SCRIPT_DIR}/test_hardware_runner"
exit $STATUS
