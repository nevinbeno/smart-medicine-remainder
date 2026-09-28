#include "../include/configuration_adapter.hpp"
#include <cstring>

EngineCell ConfigurationAdapter::convertCell(const RuntimeCell& sourceCell, uint8_t cellIndex)
{
    EngineCell target;
    target.cellIndex = cellIndex;

    strncpy(target.cellId, sourceCell.cellId, MAX_ENGINE_ID_LEN - 1);
    target.cellId[MAX_ENGINE_ID_LEN - 1] = '\0';

    target.state = sourceCell.state;
    target.currentStock = sourceCell.currentStock;
    target.hasMedicine = sourceCell.hasMedicine;

    if (sourceCell.hasMedicine)
    {
        strncpy(target.medicineId, sourceCell.medicine.medicineId, MAX_ENGINE_ID_LEN - 1);
        target.medicineId[MAX_ENGINE_ID_LEN - 1] = '\0';

        strncpy(target.medicineName, sourceCell.medicine.name, MAX_ENGINE_STRING_LEN - 1);
        target.medicineName[MAX_ENGINE_STRING_LEN - 1] = '\0';

        target.lowStockThreshold = sourceCell.medicine.lowStockThreshold;
        target.scheduleCount = sourceCell.medicine.scheduleCount;

        for (uint8_t i = 0; i < sourceCell.medicine.scheduleCount && i < MAX_SCHEDULES_PER_CELL; i++)
        {
            target.schedules[i].hour = sourceCell.medicine.schedules[i].hour;
            target.schedules[i].minute = sourceCell.medicine.schedules[i].minute;
            target.schedules[i].doseQuantity = sourceCell.medicine.schedules[i].doseQuantity;
            target.schedules[i].triggeredToday = false;
            target.schedules[i].enabled = true;
        }
    }
    else
    {
        target.lowStockThreshold = 0;
        target.scheduleCount = 0;
        target.medicineId[0] = '\0';
        target.medicineName[0] = '\0';
    }

    return target;
}

bool ConfigurationAdapter::loadIntoEngine(const RuntimeConfiguration& config, ScheduleEventEngine& engine)
{
    engine.clearCells();

    for (uint8_t i = 0; i < config.cellCount && i < MAX_ENGINE_CELLS; i++)
    {
        EngineCell converted = convertCell(config.cells[i], i);
        if (!engine.addCell(converted))
        {
            return false;
        }
    }

    return true;
}
