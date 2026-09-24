#ifndef CELL_HPP
#define CELL_HPP

#include <cstdint>

enum class CellState
{
    EMPTY,
    OCCUPIED,
    DISABLED
};

struct Cell
{
    uint8_t cellId;
    const char* assignedMedicineId;

    uint16_t currentStock;

    CellState state;
};

#endif