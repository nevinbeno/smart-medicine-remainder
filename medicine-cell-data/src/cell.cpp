#include "../include/cell.hpp"

Cell::Cell(uint8_t cellId)
    : cellId(cellId),
      assignedMedicineId(nullptr),
      currentStock(0),
      state(CellState::EMPTY)
{
}

bool Cell::assignMedicine(const char* medicineId)
{
    if (state == CellState::DISABLED || medicineId == nullptr)
    {
        return false;
    }

    assignedMedicineId = medicineId;
    state = CellState::OCCUPIED;

    return true;
}

bool Cell::removeMedicine()
{
    if (state == CellState::DISABLED)
    {
        return false;
    }

    assignedMedicineId = nullptr;
    currentStock = 0;
    state = CellState::EMPTY;

    return true;
}

bool Cell::setStock(uint16_t stock)
{
    if (state == CellState::DISABLED)
    {
        return false;
    }

    currentStock = stock;
    return true;
}

bool Cell::disable()
{
    if (state == CellState::DISABLED)
    {
        return false;
    }

    state = CellState::DISABLED;
    return true;
}

bool Cell::enable()
{
    if (state != CellState::DISABLED)
    {
        return false;
    }

    state = (assignedMedicineId == nullptr)
        ? CellState::EMPTY
        : CellState::OCCUPIED;

    return true;
}

uint8_t Cell::getCellId() const
{
    return cellId;
}

const char* Cell::getMedicineId() const
{
    return assignedMedicineId;
}

uint16_t Cell::getStock() const
{
    return currentStock;
}

CellState Cell::getState() const
{
    return state;
}

bool Cell::isEnabled() const
{
    return state != CellState::DISABLED;
}