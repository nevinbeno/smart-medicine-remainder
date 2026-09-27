#ifndef CELL_HPP
#define CELL_HPP

#include <cstdint>

enum class CellState
{
    EMPTY,
    OCCUPIED,
    DISABLED_STATE
};

class Cell
{
public:
    explicit Cell(uint8_t cellId);

    bool assignMedicine(const char* medicineId);
    bool removeMedicine();

    bool setStock(uint16_t stock);

    bool enable();
    bool disable();

    uint8_t getCellId() const;
    const char* getMedicineId() const;
    uint16_t getStock() const;
    CellState getState() const;

    bool isEnabled() const;

private:
    uint8_t cellId;
    const char* assignedMedicineId;
    uint16_t currentStock;
    CellState state;
};

#endif