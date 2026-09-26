#include "../include/configuration_loader.hpp"

#include <ArduinoJson.h>
#include <cstring>

bool loadConfiguration(
    const char* json,
    RuntimeConfiguration& configuration
)
{
    JsonDocument document;

    DeserializationError error = deserializeJson(document, json);

    if (error)
    {
        return false;
    }

    JsonArray cells = document["cells"].as<JsonArray>();

    uint8_t cellCount = document["cellCount"] | 0;

    if (cellCount > MAX_CELLS)
    {
        return false;
    }

    configuration.cellCount = cellCount;

    for (uint8_t i = 0; i < cellCount; i++)
    {
        JsonObject jsonCell = cells[i];

        RuntimeCell& cell = configuration.cells[i];

        // Cell ID
        const char* cellId = jsonCell["cellId"] | "";

        std::strncpy(
            cell.cellId,
            cellId,
            MAX_CELL_ID_LENGTH - 1
        );

        cell.cellId[MAX_CELL_ID_LENGTH - 1] = '\0';

        // Cell state
        const char* state = jsonCell["state"] | "EMPTY";

        if (std::strcmp(state, "OCCUPIED") == 0)
        {
            cell.state = CellState::OCCUPIED;
        }
        else if (std::strcmp(state, "DISABLED_STATE") == 0)
        {
            cell.state = CellState::DISABLED_STATE;
        }
        else
        {
            cell.state = CellState::EMPTY;
        }

        // Stock
        cell.currentStock = jsonCell["currentStock"] | 0;

        // Medicine
        JsonObject jsonMedicine = jsonCell["medicine"];

        if (jsonMedicine.isNull())
        {
            cell.hasMedicine = false;
            continue;
        }

        cell.hasMedicine = true;

        RuntimeMedicine& medicine = cell.medicine;

        const char* medicineId =
            jsonMedicine["medicineId"] | "";

        std::strncpy(
            medicine.medicineId,
            medicineId,
            MAX_MEDICINE_ID_LENGTH - 1
        );

        medicine.medicineId[MAX_MEDICINE_ID_LENGTH - 1] = '\0';

        const char* name =
            jsonMedicine["name"] | "";

        std::strncpy(
            medicine.name,
            name,
            MAX_MEDICINE_NAME_LENGTH - 1
        );

        medicine.name[MAX_MEDICINE_NAME_LENGTH - 1] = '\0';

        medicine.lowStockThreshold =
            jsonMedicine["lowStockThreshold"] | 0;

        // Schedules
        JsonArray schedules =
            jsonMedicine["schedules"].as<JsonArray>();

        uint8_t scheduleCount = schedules.size();

        if (scheduleCount > MAX_SCHEDULES)
        {
            scheduleCount = MAX_SCHEDULES;
        }

        medicine.scheduleCount = scheduleCount;

        for (uint8_t j = 0; j < scheduleCount; j++)
        {
            JsonObject jsonSchedule = schedules[j];

            medicine.schedules[j].hour =
                jsonSchedule["hour"] | 0;

            medicine.schedules[j].minute =
                jsonSchedule["minute"] | 0;

            medicine.schedules[j].doseQuantity =
                jsonSchedule["doseQuantity"] | 0;
        }
    }

    return true;
}