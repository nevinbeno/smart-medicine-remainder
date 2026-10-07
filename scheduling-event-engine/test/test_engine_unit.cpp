#include <iostream>
#include <cassert>
#include <vector>
#include <string>

#include "../include/schedule_event_engine.hpp"
#include "../include/configuration_adapter.hpp"

// Global callback recording
struct EventRecord
{
    EventType type;
    EventState state;
    std::string cellId;
    std::string medicineName;
    uint16_t doseQuantity;
    TimeOfDay scheduledTime;
};

static std::vector<EventRecord> g_recordedEvents;

static void testEventCallback(const DoseEvent& event, void* /*userData*/)
{
    EventRecord record;
    record.type = event.type;
    record.state = event.state;
    record.cellId = event.cellId;
    record.medicineName = event.medicineName;
    record.doseQuantity = event.doseQuantity;
    record.scheduledTime = event.scheduledTime;
    g_recordedEvents.push_back(record);
}

static void clearTestRecords()
{
    g_recordedEvents.clear();
}

// ----------------------------------------------------------------------------
// Test 1: Single cell scheduled dose triggers at exact time
// ----------------------------------------------------------------------------
void test_single_schedule_trigger()
{
    std::cout << "[RUN] test_single_schedule_trigger..." << std::endl;
    clearTestRecords();

    SimulatedTimeProvider timeProvider(7, 59, 50);
    ScheduleEventEngine engine(&timeProvider);
    engine.registerEventCallback(testEventCallback);

    EngineCell cell;
    strncpy(cell.cellId, "CELL01", sizeof(cell.cellId));
    cell.state = CellState::OCCUPIED;
    cell.hasMedicine = true;
    strncpy(cell.medicineId, "MED001", sizeof(cell.medicineId));
    strncpy(cell.medicineName, "Paracetamol", sizeof(cell.medicineName));
    cell.currentStock = 20;
    cell.lowStockThreshold = 3;
    cell.scheduleCount = 1;
    cell.schedules[0] = DoseSchedule(8, 0, 1);

    assert(engine.addCell(cell));

    // Update at 07:59:50 -> should not trigger
    engine.update();
    assert(engine.getDueEventCount() == 0);
    assert(g_recordedEvents.empty());

    // Advance to 08:00:00 -> should trigger DOSE_DUE
    timeProvider.advanceSeconds(10);
    engine.update();

    assert(engine.getDueEventCount() == 1);
    assert(engine.hasActiveDueEvents());
    assert(engine.isCellDue(0));
    assert(g_recordedEvents.size() == 1);
    assert(g_recordedEvents[0].type == EventType::DOSE_DUE);
    assert(g_recordedEvents[0].state == EventState::DUE);
    assert(g_recordedEvents[0].cellId == "CELL01");
    assert(g_recordedEvents[0].medicineName == "Paracetamol");

    std::cout << "  [PASS] Single schedule triggered at 08:00:00" << std::endl;
}

// ----------------------------------------------------------------------------
// Test 2: Simultaneous schedules across multiple cells
// ----------------------------------------------------------------------------
void test_simultaneous_schedules()
{
    std::cout << "[RUN] test_simultaneous_schedules..." << std::endl;
    clearTestRecords();

    SimulatedTimeProvider timeProvider(7, 59, 0);
    ScheduleEventEngine engine(&timeProvider);
    engine.registerEventCallback(testEventCallback);

    // Cell 0: Paracetamol at 08:00
    EngineCell cell0;
    strncpy(cell0.cellId, "CELL01", sizeof(cell0.cellId));
    cell0.state = CellState::OCCUPIED;
    cell0.hasMedicine = true;
    strncpy(cell0.medicineName, "Paracetamol", sizeof(cell0.medicineName));
    cell0.scheduleCount = 1;
    cell0.schedules[0] = DoseSchedule(8, 0, 1);
    engine.addCell(cell0);

    // Cell 1: Amoxicillin at 08:00 (simultaneous!)
    EngineCell cell1;
    strncpy(cell1.cellId, "CELL02", sizeof(cell1.cellId));
    cell1.state = CellState::OCCUPIED;
    cell1.hasMedicine = true;
    strncpy(cell1.medicineName, "Amoxicillin", sizeof(cell1.medicineName));
    cell1.scheduleCount = 1;
    cell1.schedules[0] = DoseSchedule(8, 0, 2);
    engine.addCell(cell1);

    // Cell 2: Cetirizine at 22:00 (not now)
    EngineCell cell2;
    strncpy(cell2.cellId, "CELL03", sizeof(cell2.cellId));
    cell2.state = CellState::OCCUPIED;
    cell2.hasMedicine = true;
    strncpy(cell2.medicineName, "Cetirizine", sizeof(cell2.medicineName));
    cell2.scheduleCount = 1;
    cell2.schedules[0] = DoseSchedule(22, 0, 1);
    engine.addCell(cell2);

    // Advance to 08:00
    timeProvider.advanceMinutes(1);
    engine.update();

    // Both Cell 0 and Cell 1 should be DUE
    assert(engine.getDueEventCount() == 2);
    assert(engine.isCellDue(0));
    assert(engine.isCellDue(1));
    assert(!engine.isCellDue(2));
    assert(g_recordedEvents.size() == 2);

    std::cout << "  [PASS] Multiple simultaneous medicines detected at 08:00 (Count: 2)" << std::endl;
}

// ----------------------------------------------------------------------------
// Test 3: Dose confirmation during DUE window
// ----------------------------------------------------------------------------
void test_dose_confirmation()
{
    std::cout << "[RUN] test_dose_confirmation..." << std::endl;
    clearTestRecords();

    SimulatedTimeProvider timeProvider(8, 0, 0);
    ScheduleEventEngine engine(&timeProvider);
    engine.registerEventCallback(testEventCallback);

    EngineCell cell;
    strncpy(cell.cellId, "CELL01", sizeof(cell.cellId));
    cell.state = CellState::OCCUPIED;
    cell.hasMedicine = true;
    strncpy(cell.medicineName, "Paracetamol", sizeof(cell.medicineName));
    cell.scheduleCount = 1;
    cell.schedules[0] = DoseSchedule(8, 0, 1);
    engine.addCell(cell);

    engine.update();
    assert(engine.getDueEventCount() == 1);

    // Patient presses confirm button for Cell 0
    bool confirmed = engine.confirmDose(0);
    assert(confirmed);
    assert(engine.getDueEventCount() == 0);
    assert(!engine.hasActiveDueEvents());

    // Verify DOSE_CONFIRMED callback emitted
    assert(g_recordedEvents.size() == 2); // DOSE_DUE, then DOSE_CONFIRMED
    assert(g_recordedEvents[1].type == EventType::DOSE_CONFIRMED);
    assert(g_recordedEvents[1].state == EventState::CONFIRMED);

    std::cout << "  [PASS] Dose confirmed successfully during due window" << std::endl;
}

// ----------------------------------------------------------------------------
// Test 4: Grace period expiration -> Transition to MISSED
// ----------------------------------------------------------------------------
void test_grace_period_missed_dose()
{
    std::cout << "[RUN] test_grace_period_missed_dose..." << std::endl;
    clearTestRecords();

    SimulatedTimeProvider timeProvider(8, 0, 0);
    ScheduleEventEngine engine(&timeProvider);
    engine.setGracePeriodMinutes(10); // 10 minute grace period
    engine.registerEventCallback(testEventCallback);

    EngineCell cell;
    strncpy(cell.cellId, "CELL01", sizeof(cell.cellId));
    cell.state = CellState::OCCUPIED;
    cell.hasMedicine = true;
    strncpy(cell.medicineName, "Paracetamol", sizeof(cell.medicineName));
    cell.scheduleCount = 1;
    cell.schedules[0] = DoseSchedule(8, 0, 1);
    engine.addCell(cell);

    engine.update();
    assert(engine.getDueEventCount() == 1);
    assert(engine.getMissedEventCount() == 0);

    // Advance 5 minutes (within grace period)
    timeProvider.advanceMinutes(5);
    engine.update();
    assert(engine.getDueEventCount() == 1);
    assert(engine.getMissedEventCount() == 0);

    // Advance 6 more minutes (total 11 minutes -> exceeds 10 min grace period)
    timeProvider.advanceMinutes(6);
    engine.update();

    // Event should transition to MISSED
    assert(engine.getDueEventCount() == 0);
    assert(engine.getMissedEventCount() == 1);
    assert(engine.hasActiveMissedEvents());
    assert(engine.isCellMissed(0));

    // Callback DOSE_MISSED received
    assert(g_recordedEvents.size() == 2); // DOSE_DUE, then DOSE_MISSED
    assert(g_recordedEvents[1].type == EventType::DOSE_MISSED);
    assert(g_recordedEvents[1].state == EventState::MISSED);

    std::cout << "  [PASS] Grace period expired; transitioned to MISSED state" << std::endl;
}

// ----------------------------------------------------------------------------
// Test 5: Dismiss missed dose without reducing stock (Clock app style)
// ----------------------------------------------------------------------------
void test_dismiss_missed_dose_no_stock_decrement()
{
    std::cout << "[RUN] test_dismiss_missed_dose_no_stock_decrement..." << std::endl;
    clearTestRecords();

    SimulatedTimeProvider timeProvider(8, 0, 0);
    ScheduleEventEngine engine(&timeProvider);
    engine.setGracePeriodMinutes(5);
    engine.registerEventCallback(testEventCallback);

    EngineCell cell;
    strncpy(cell.cellId, "CELL01", sizeof(cell.cellId));
    cell.state = CellState::OCCUPIED;
    cell.hasMedicine = true;
    cell.currentStock = 20;
    strncpy(cell.medicineName, "Paracetamol", sizeof(cell.medicineName));
    cell.scheduleCount = 1;
    cell.schedules[0] = DoseSchedule(8, 0, 1);
    engine.addCell(cell);

    engine.update();

    // Advance time past grace period
    timeProvider.advanceMinutes(6);
    engine.update();
    assert(engine.getMissedEventCount() == 1);

    // Patient presses confirm button to dismiss reminder
    bool handled = engine.handleCellButtonPress(0);
    assert(handled);

    // Missed list should be cleared
    assert(engine.getMissedEventCount() == 0);
    assert(!engine.hasActiveMissedEvents());

    // Callback emitted should be DOSE_DISMISSED (NOT DOSE_CONFIRMED)
    assert(g_recordedEvents.size() == 3); // DUE, MISSED, DISMISSED
    assert(g_recordedEvents[2].type == EventType::DOSE_DISMISSED);
    assert(g_recordedEvents[2].state == EventState::DISMISSED);

    // Stock must remain unchanged (20)
    assert(engine.getCell(0)->currentStock == 20);

    std::cout << "  [PASS] Missed dose dismissed properly; stock preserved at 20" << std::endl;
}

// ----------------------------------------------------------------------------
// Test 6: Dismiss all missed doses when any cell button pressed
// ----------------------------------------------------------------------------
void test_dismiss_all_missed_on_any_button()
{
    std::cout << "[RUN] test_dismiss_all_missed_on_any_button..." << std::endl;
    clearTestRecords();

    SimulatedTimeProvider timeProvider(8, 0, 0);
    ScheduleEventEngine engine(&timeProvider);
    engine.setGracePeriodMinutes(5);

    // Add 2 cells with dose at 08:00
    EngineCell cell0;
    strncpy(cell0.cellId, "CELL01", sizeof(cell0.cellId));
    cell0.state = CellState::OCCUPIED;
    cell0.hasMedicine = true;
    cell0.scheduleCount = 1;
    cell0.schedules[0] = DoseSchedule(8, 0, 1);
    engine.addCell(cell0);

    EngineCell cell1;
    strncpy(cell1.cellId, "CELL02", sizeof(cell1.cellId));
    cell1.state = CellState::OCCUPIED;
    cell1.hasMedicine = true;
    cell1.scheduleCount = 1;
    cell1.schedules[0] = DoseSchedule(8, 0, 1);
    engine.addCell(cell1);

    engine.update();
    timeProvider.advanceMinutes(10);
    engine.update();

    assert(engine.getMissedEventCount() == 2);

    // User presses confirm button on cell 0 to clear missed alerts
    uint8_t dismissed = engine.dismissAllMissedDoses();
    assert(dismissed == 2);
    assert(engine.getMissedEventCount() == 0);

    std::cout << "  [PASS] All missed alerts cleared simultaneously" << std::endl;
}

// ----------------------------------------------------------------------------
// Test 7: Prevent duplicate triggers within the same minute
// ----------------------------------------------------------------------------
void test_prevent_duplicate_triggers()
{
    std::cout << "[RUN] test_prevent_duplicate_triggers..." << std::endl;
    clearTestRecords();

    SimulatedTimeProvider timeProvider(8, 0, 10);
    ScheduleEventEngine engine(&timeProvider);
    engine.registerEventCallback(testEventCallback);

    EngineCell cell;
    strncpy(cell.cellId, "CELL01", sizeof(cell.cellId));
    cell.state = CellState::OCCUPIED;
    cell.hasMedicine = true;
    cell.scheduleCount = 1;
    cell.schedules[0] = DoseSchedule(8, 0, 1);
    engine.addCell(cell);

    // Run update at 08:00:10
    engine.update();
    assert(engine.getDueEventCount() == 1);
    assert(g_recordedEvents.size() == 1);

    // Run update again at 08:00:20 (same minute)
    timeProvider.advanceSeconds(10);
    engine.update();
    assert(engine.getDueEventCount() == 1);
    assert(g_recordedEvents.size() == 1); // No new event

    // Run update again at 08:00:50 (same minute)
    timeProvider.advanceSeconds(30);
    engine.update();
    assert(engine.getDueEventCount() == 1);
    assert(g_recordedEvents.size() == 1); // Still no duplicate

    std::cout << "  [PASS] No duplicate triggers inside same scheduled minute" << std::endl;
}

// ----------------------------------------------------------------------------
// Test 8: Midnight rollover and daily schedule reset
// ----------------------------------------------------------------------------
void test_midnight_rollover()
{
    std::cout << "[RUN] test_midnight_rollover..." << std::endl;
    clearTestRecords();

    SimulatedTimeProvider timeProvider(23, 59, 50);
    ScheduleEventEngine engine(&timeProvider);
    engine.registerEventCallback(testEventCallback);

    EngineCell cell;
    strncpy(cell.cellId, "CELL01", sizeof(cell.cellId));
    cell.state = CellState::OCCUPIED;
    cell.hasMedicine = true;
    cell.scheduleCount = 1;
    cell.schedules[0] = DoseSchedule(8, 0, 1);
    cell.schedules[0].triggeredToday = true; // Was triggered yesterday
    engine.addCell(cell);

    engine.update();
    assert(engine.getCell(0)->schedules[0].triggeredToday == true);

    // Advance 20 seconds -> 00:00:10 next day
    timeProvider.advanceSeconds(20);
    engine.update();

    // Verify daily trigger flag was reset to false
    assert(engine.getCell(0)->schedules[0].triggeredToday == false);

    // Verify DAILY_RESET event emitted
    bool foundReset = false;
    for (const auto& rec : g_recordedEvents)
    {
        if (rec.type == EventType::DAILY_RESET)
        {
            foundReset = true;
            break;
        }
    }
    assert(foundReset);

    std::cout << "  [PASS] Midnight rollover reset triggeredToday flags and emitted DAILY_RESET" << std::endl;
}

// ----------------------------------------------------------------------------
// Test 9: Scalability from 1 cell to 16 cells (4x4)
// ----------------------------------------------------------------------------
void test_scalability_16_cells()
{
    std::cout << "[RUN] test_scalability_16_cells..." << std::endl;
    SimulatedTimeProvider timeProvider(12, 0, 0);
    ScheduleEventEngine engine(&timeProvider);

    for (uint8_t i = 0; i < 16; i++)
    {
        EngineCell cell;
        snprintf(cell.cellId, sizeof(cell.cellId), "CELL%02d", i + 1);
        cell.state = CellState::OCCUPIED;
        cell.hasMedicine = true;
        snprintf(cell.medicineName, sizeof(cell.medicineName), "Med_%d", i + 1);
        cell.scheduleCount = 1;
        cell.schedules[0] = DoseSchedule(12, 0, 1);
        assert(engine.addCell(cell));
    }

    assert(engine.getCellCount() == 16);

    engine.update();
    assert(engine.getDueEventCount() == 16);

    std::cout << "  [PASS] Scalability verified: 16 simultaneous cells handled cleanly" << std::endl;
}

// ----------------------------------------------------------------------------
// Test 10: Configuration Adapter
// ----------------------------------------------------------------------------
void test_configuration_adapter()
{
    std::cout << "[RUN] test_configuration_adapter..." << std::endl;

    RuntimeConfiguration config;
    config.cellCount = 4;

    for (uint8_t i = 0; i < 4; i++)
    {
        snprintf(config.cells[i].cellId, sizeof(config.cells[i].cellId), "CELL%02d", i + 1);
        config.cells[i].state = CellState::OCCUPIED;
        config.cells[i].hasMedicine = true;
        config.cells[i].currentStock = 10 + i;
        snprintf(config.cells[i].medicine.medicineId, sizeof(config.cells[i].medicine.medicineId), "MED00%d", i + 1);
        snprintf(config.cells[i].medicine.name, sizeof(config.cells[i].medicine.name), "Medicine %d", i + 1);
        config.cells[i].medicine.lowStockThreshold = 3;
        config.cells[i].medicine.scheduleCount = 2;
        config.cells[i].medicine.schedules[0] = ScheduleEntry{8, 0, 1};
        config.cells[i].medicine.schedules[1] = ScheduleEntry{20, 0, 1};
    }

    SimulatedTimeProvider timeProvider(8, 0, 0);
    ScheduleEventEngine engine(&timeProvider);

    assert(ConfigurationAdapter::loadIntoEngine(config, engine));
    assert(engine.getCellCount() == 4);

    engine.update();
    assert(engine.getDueEventCount() == 4);

    std::cout << "  [PASS] Configuration adapter successfully loaded 4 cells into engine" << std::endl;
}

// ----------------------------------------------------------------------------
// Test 11: Next Scheduled Dose Query
// ----------------------------------------------------------------------------
void test_next_scheduled_dose()
{
    std::cout << "[RUN] test_next_scheduled_dose..." << std::endl;

    SimulatedTimeProvider timeProvider(10, 0, 0);
    ScheduleEventEngine engine(&timeProvider);

    EngineCell cell;
    strncpy(cell.cellId, "CELL01", sizeof(cell.cellId));
    cell.state = CellState::OCCUPIED;
    cell.hasMedicine = true;
    cell.scheduleCount = 2;
    cell.schedules[0] = DoseSchedule(8, 0, 1);   // Earlier
    cell.schedules[1] = DoseSchedule(14, 30, 1); // Later today
    engine.addCell(cell);

    TimeOfDay nextTime;
    uint8_t nextCellIdx = 0;
    bool found = engine.getNextScheduledDose(nextTime, nextCellIdx);

    assert(found);
    assert(nextTime.hour == 14);
    assert(nextTime.minute == 30);
    assert(nextCellIdx == 0);

    std::cout << "  [PASS] Next scheduled dose correctly identified as 14:30" << std::endl;
}

// ----------------------------------------------------------------------------
// Test 12: Stock management: decrement on confirmation, block due when stock is 0
// ----------------------------------------------------------------------------
void test_stock_zero_and_decrement()
{
    std::cout << "[RUN] test_stock_zero_and_decrement..." << std::endl;
    clearTestRecords();

    SimulatedTimeProvider timeProvider(8, 0, 0);
    ScheduleEventEngine engine(&timeProvider);
    engine.registerEventCallback(testEventCallback);

    EngineCell cell;
    strncpy(cell.cellId, "CELL01", sizeof(cell.cellId));
    cell.state = CellState::OCCUPIED;
    cell.hasMedicine = true;
    strncpy(cell.medicineName, "Paracetamol", sizeof(cell.medicineName));
    cell.currentStock = 1; // Only 1 unit in inventory
    cell.lowStockThreshold = 1;
    cell.scheduleCount = 2;
    cell.schedules[0] = DoseSchedule(8, 0, 1);
    cell.schedules[1] = DoseSchedule(9, 0, 1);
    engine.addCell(cell);

    // 08:00 -> Due dose triggers because stock == 1
    engine.update();
    assert(engine.getDueEventCount() == 1);
    assert(engine.getCell(0)->currentStock == 1);

    // Confirm dose -> Stock decrements from 1 to 0
    bool ok = engine.confirmDose(0);
    assert(ok);
    assert(engine.getCell(0)->currentStock == 0);
    assert(engine.getDueEventCount() == 0);

    // Manual triggerDueEvent must be rejected when stock is 0
    assert(engine.triggerDueEvent(0) == false);

    // Clear recorded events and advance to 09:00:00 (second schedule)
    clearTestRecords();
    timeProvider.advanceSeconds(3600); // Now 09:00:00
    engine.update();

    // Since stock is 0, DOSE_DUE must NOT be placed into active due events!
    assert(engine.getDueEventCount() == 0);
    assert(!engine.hasActiveDueEvents());

    // Instead, LOW_STOCK_WARNING with doseQuantity == 0 (out of stock) should be emitted
    assert(g_recordedEvents.size() >= 1);
    bool sawOutOfStock = false;
    for (const auto& ev : g_recordedEvents)
    {
        if (ev.type == EventType::LOW_STOCK_WARNING && ev.doseQuantity == 0)
        {
            sawOutOfStock = true;
            break;
        }
    }
    assert(sawOutOfStock);

    // Refill stock to 5 units
    assert(engine.updateCellStock(0, 5));
    assert(engine.getCell(0)->currentStock == 5);

    // Now manual triggerDueEvent should succeed
    assert(engine.triggerDueEvent(0) == true);
    assert(engine.getDueEventCount() == 1);

    std::cout << "  [PASS] Stock properly decrements to 0, blocks DUE when empty, and resumes on refill" << std::endl;
}

// ----------------------------------------------------------------------------
// Main test runner
// ----------------------------------------------------------------------------
int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << " SCHEDULING & TIME/EVENT ENGINE TESTS" << std::endl;
    std::cout << "========================================" << std::endl;

    test_single_schedule_trigger();
    test_simultaneous_schedules();
    test_dose_confirmation();
    test_grace_period_missed_dose();
    test_dismiss_missed_dose_no_stock_decrement();
    test_dismiss_all_missed_on_any_button();
    test_prevent_duplicate_triggers();
    test_midnight_rollover();
    test_scalability_16_cells();
    test_configuration_adapter();
    test_next_scheduled_dose();
    test_stock_zero_and_decrement();

    std::cout << "========================================" << std::endl;
    std::cout << " ALL 12 UNIT TESTS PASSED SUCCESSFULLY!" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
