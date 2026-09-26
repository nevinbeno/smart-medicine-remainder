#include <Arduino.h>

#include "../medicine-cell-data/include/configuration_loader.hpp"


// Embedded configuration.json
extern const uint8_t configuration_json_start[]
    asm("_binary_medicine_cell_data_config_configuration_json_start");

extern const uint8_t configuration_json_end[]
    asm("_binary_medicine_cell_data_config_configuration_json_end");


void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("================================");
    Serial.println(" SMART MEDICINE CONFIGURATION");
    Serial.println("================================");


    // Get embedded JSON
    const char* json =
        reinterpret_cast<const char*>(
            configuration_json_start
        );


    // Runtime configuration
    RuntimeConfiguration configuration;


    // Load JSON into C++ structures
    if (!loadConfiguration(json, configuration))
    {
        Serial.println("ERROR: Configuration loading failed!");
        return;
    }


    Serial.println("Configuration loaded successfully!");
    Serial.println();


    // Display configuration
    Serial.print("Cell count: ");
    Serial.println(configuration.cellCount);

    Serial.println();


    for (uint8_t i = 0;
         i < configuration.cellCount;
         i++)
    {
        RuntimeCell& cell =
            configuration.cells[i];


        Serial.println("----------------------------");

        Serial.print("Cell ID: ");
        Serial.println(cell.cellId);


        Serial.print("State: ");

        if (cell.state == CellState::OCCUPIED)
        {
            Serial.println("OCCUPIED");
        }
        else if (cell.state == CellState::DISABLED_STATE)
        {
            Serial.println("DISABLED");
        }
        else
        {
            Serial.println("EMPTY");
        }


        Serial.print("Current stock: ");
        Serial.println(cell.currentStock);


        if (!cell.hasMedicine)
        {
            Serial.println("Medicine: None");
            continue;
        }


        Serial.print("Medicine ID: ");
        Serial.println(
            cell.medicine.medicineId
        );


        Serial.print("Medicine name: ");
        Serial.println(
            cell.medicine.name
        );


        Serial.print("Low-stock threshold: ");
        Serial.println(
            cell.medicine.lowStockThreshold
        );


        Serial.print("Schedule count: ");
        Serial.println(
            cell.medicine.scheduleCount
        );


        for (uint8_t j = 0;
             j < cell.medicine.scheduleCount;
             j++)
        {
            ScheduleEntry& schedule =
                cell.medicine.schedules[j];


            Serial.print("  ");

            if (schedule.hour < 10)
                Serial.print("0");

            Serial.print(schedule.hour);

            Serial.print(":");

            if (schedule.minute < 10)
                Serial.print("0");

            Serial.print(schedule.minute);

            Serial.print(" -> dose ");

            Serial.println(
                schedule.doseQuantity
            );
        }
    }


    Serial.println();
    Serial.println("================================");
    Serial.println(" Configuration test completed");
    Serial.println("================================");
}


void loop()
{
    // Nothing required for this test.
}