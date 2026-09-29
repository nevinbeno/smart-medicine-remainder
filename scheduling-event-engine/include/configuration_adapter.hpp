#ifndef CONFIGURATION_ADAPTER_HPP
#define CONFIGURATION_ADAPTER_HPP

#include "schedule_event_engine.hpp"

// Check if medicine-cell-data headers are available, or provide compatible layout
#if __has_include("../../../medicine-cell-data/include/configuration.hpp")
    #include "../../../medicine-cell-data/include/configuration.hpp"
    #define HAS_EXTERNAL_CONFIG_HPP 1
#elif __has_include("configuration.hpp")
    #include "configuration.hpp"
    #define HAS_EXTERNAL_CONFIG_HPP 1
#else
    #define HAS_EXTERNAL_CONFIG_HPP 0

    constexpr uint8_t ADAPTER_MAX_CELLS = 16;
    constexpr uint8_t ADAPTER_MAX_SCHEDULES = 8;
    constexpr uint8_t ADAPTER_MAX_MED_ID_LEN = 16;
    constexpr uint8_t ADAPTER_MAX_MED_NAME_LEN = 32;
    constexpr uint8_t ADAPTER_MAX_CELL_ID_LEN = 16;

    struct ScheduleEntry
    {
        uint8_t hour;
        uint8_t minute;
        uint16_t doseQuantity;
    };

    struct RuntimeMedicine
    {
        char medicineId[ADAPTER_MAX_MED_ID_LEN];
        char name[ADAPTER_MAX_MED_NAME_LEN];
        uint16_t lowStockThreshold;
        ScheduleEntry schedules[ADAPTER_MAX_SCHEDULES];
        uint8_t scheduleCount;
    };

    struct RuntimeCell
    {
        char cellId[ADAPTER_MAX_CELL_ID_LEN];
        CellState state;
        uint16_t currentStock;
        RuntimeMedicine medicine;
        bool hasMedicine;
    };

    struct RuntimeConfiguration
    {
        uint8_t cellCount;
        RuntimeCell cells[ADAPTER_MAX_CELLS];
    };
#endif

/**
 * @brief Adapter for converting RuntimeConfiguration from medicine-cell-data
 * into the engine's internal EngineCell format.
 */
class ConfigurationAdapter
{
public:
    /**
     * @brief Populate a ScheduleEventEngine from a RuntimeConfiguration.
     * @param config The runtime configuration snapshot.
     * @param engine Target ScheduleEventEngine instance.
     * @return true if loading succeeded.
     */
    static bool loadIntoEngine(const RuntimeConfiguration& config, ScheduleEventEngine& engine);

    /**
     * @brief Convert a single RuntimeCell into an EngineCell.
     */
    static EngineCell convertCell(const RuntimeCell& sourceCell, uint8_t cellIndex);
};

#endif // CONFIGURATION_ADAPTER_HPP
