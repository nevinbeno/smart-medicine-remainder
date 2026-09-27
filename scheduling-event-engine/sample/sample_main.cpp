#include <iostream>
#include <iomanip>
#include "../include/schedule_event_engine.hpp"
#include "../include/time_provider.hpp"

// Simulated downstream module callbacks (OLED UI, Hardware Alert, Stock Manager)
void onEngineEvent(const DoseEvent& event, void* /*userData*/)
{
    std::cout << "\n>>> [EVENT RECEIVED] ";
    switch (event.type)
    {
    case EventType::DOSE_DUE:
        std::cout << "EVENT_DOSE_DUE" << std::endl;
        std::cout << "    [OLED UI] Displaying Reminder: " << event.medicineName
                  << " (Cell: " << event.cellId << ", Qty: " << event.doseQuantity << ")" << std::endl;
        std::cout << "    [ALERT HARDWARE] Cell " << event.cellId << " LED -> RED" << std::endl;
        std::cout << "    [ALERT HARDWARE] Alert Buzzer -> BEEPING..." << std::endl;
        break;

    case EventType::DOSE_CONFIRMED:
        std::cout << "EVENT_DOSE_CONFIRMED" << std::endl;
        std::cout << "    [ALERT HARDWARE] Cell " << event.cellId << " LED -> GREEN (for 1 sec)" << std::endl;
        std::cout << "    [ALERT HARDWARE] Alert Buzzer -> OFF" << std::endl;
        std::cout << "    [STOCK MANAGER] Confirmed! Decrementing stock for " << event.medicineName << std::endl;
        break;

    case EventType::DOSE_MISSED:
        std::cout << "EVENT_DOSE_MISSED (Clock App Reminder Style)" << std::endl;
        std::cout << "    [OLED UI] Persistent Banner: MISSED DOSE - " << event.medicineName
                  << " scheduled for " << std::setfill('0') << std::setw(2) << (int)event.scheduledTime.hour
                  << ":" << std::setw(2) << (int)event.scheduledTime.minute << std::endl;
        std::cout << "    [ALERT HARDWARE] Audible Buzzer stopped. Reminder pending user action." << std::endl;
        break;

    case EventType::DOSE_DISMISSED:
        std::cout << "EVENT_DOSE_DISMISSED" << std::endl;
        std::cout << "    [OLED UI] Cleared missed dose banner for " << event.medicineName << std::endl;
        std::cout << "    [STOCK MANAGER] Note: Stock is NOT decremented for missed dose dismissal." << std::endl;
        break;

    case EventType::DAILY_RESET:
        std::cout << "EVENT_DAILY_RESET" << std::endl;
        std::cout << "    [ENGINE] Midnight rollover complete. All daily schedules refreshed for today." << std::endl;
        break;

    case EventType::LOW_STOCK_WARNING:
        std::cout << "EVENT_LOW_STOCK_WARNING" << std::endl;
        std::cout << "    [ALERT HARDWARE] Warning Buzzer -> Intermittent warning beep!" << std::endl;
        std::cout << "    [ALERT HARDWARE] Cell " << event.cellId << " LED -> YELLOW" << std::endl;
        break;

    default:
        std::cout << "UNKNOWN EVENT" << std::endl;
        break;
    }
}

int main()
{
    std::cout << "========================================================" << std::endl;
    std::cout << " SMART MEDICINE REMINDER - SCHEDULING ENGINE DEMO" << std::endl;
    std::cout << "========================================================" << std::endl;

    // 1. Initialize simulated time provider at 07:59:55
    SimulatedTimeProvider timeProvider(7, 59, 55);
    ScheduleEventEngine engine(&timeProvider);
    engine.setGracePeriodMinutes(5); // 5-minute grace window for alerts

    // 2. Register downstream callback handler
    engine.registerEventCallback(onEngineEvent);

    // 3. Configure 4 cells (2x2 modular matrix)
    // Cell 1: Paracetamol (Due at 08:00 and 20:00)
    EngineCell cell1;
    strncpy(cell1.cellId, "CELL01", sizeof(cell1.cellId));
    cell1.state = CellState::OCCUPIED;
    cell1.hasMedicine = true;
    cell1.currentStock = 20;
    cell1.lowStockThreshold = 3;
    strncpy(cell1.medicineId, "MED001", sizeof(cell1.medicineId));
    strncpy(cell1.medicineName, "Paracetamol", sizeof(cell1.medicineName));
    cell1.scheduleCount = 2;
    cell1.schedules[0] = DoseSchedule(8, 0, 1);
    cell1.schedules[1] = DoseSchedule(20, 0, 1);
    engine.addCell(cell1);

    // Cell 2: Cetirizine (Due at 22:00)
    EngineCell cell2;
    strncpy(cell2.cellId, "CELL02", sizeof(cell2.cellId));
    cell2.state = CellState::OCCUPIED;
    cell2.hasMedicine = true;
    cell2.currentStock = 10;
    cell2.lowStockThreshold = 2;
    strncpy(cell2.medicineId, "MED002", sizeof(cell2.medicineId));
    strncpy(cell2.medicineName, "Cetirizine", sizeof(cell2.medicineName));
    cell2.scheduleCount = 1;
    cell2.schedules[0] = DoseSchedule(22, 0, 1);
    engine.addCell(cell2);

    // Cell 3: Amoxicillin (Due at 08:00 - Simultaneous with Cell 1!)
    EngineCell cell3;
    strncpy(cell3.cellId, "CELL03", sizeof(cell3.cellId));
    cell3.state = CellState::OCCUPIED;
    cell3.hasMedicine = true;
    cell3.currentStock = 15;
    cell3.lowStockThreshold = 3;
    strncpy(cell3.medicineId, "MED003", sizeof(cell3.medicineId));
    strncpy(cell3.medicineName, "Amoxicillin", sizeof(cell3.medicineName));
    cell3.scheduleCount = 1;
    cell3.schedules[0] = DoseSchedule(8, 0, 1);
    engine.addCell(cell3);

    // Cell 4: Pantocid (Due at 07:00)
    EngineCell cell4;
    strncpy(cell4.cellId, "CELL04", sizeof(cell4.cellId));
    cell4.state = CellState::OCCUPIED;
    cell4.hasMedicine = true;
    cell4.currentStock = 8;
    cell4.lowStockThreshold = 3;
    strncpy(cell4.medicineId, "MED004", sizeof(cell4.medicineId));
    strncpy(cell4.medicineName, "Pantocid", sizeof(cell4.medicineName));
    cell4.scheduleCount = 1;
    cell4.schedules[0] = DoseSchedule(7, 0, 1);
    engine.addCell(cell4);

    std::cout << "\nEngine initialized with " << (int)engine.getCellCount() << " cells." << std::endl;
    std::cout << "Current Simulated Time: 07:59:55" << std::endl;

    // Check before 08:00
    engine.update();
    std::cout << "Active Due Doses: " << (int)engine.getDueEventCount() << std::endl;

    // Step 1: Advance 5 seconds to 08:00:00 -> Simultaneous dose triggers!
    std::cout << "\n--- Advancing time by 5 seconds to 08:00:00 ---" << std::endl;
    timeProvider.advanceSeconds(5);
    engine.update();

    std::cout << "\nActive Due Doses now: " << (int)engine.getDueEventCount() << std::endl;
    std::cout << "Multiple medicines due at once: " << (engine.getDueEventCount() > 1 ? "YES" : "NO") << std::endl;

    // Step 2: Patient confirms Cell 1 (Paracetamol)
    std::cout << "\n--- Patient presses CONFIRM button on CELL01 ---" << std::endl;
    engine.confirmDose("CELL01");
    std::cout << "Remaining Due Doses: " << (int)engine.getDueEventCount() << std::endl;

    // Step 3: Time passes without confirming Cell 3 (Amoxicillin) -> Grace period expires!
    std::cout << "\n--- Advancing time by 6 minutes to 08:06:00 (Grace period was 5 min) ---" << std::endl;
    timeProvider.advanceMinutes(6);
    engine.update();

    std::cout << "\nActive Due Doses: " << (int)engine.getDueEventCount() << std::endl;
    std::cout << "Active Missed Reminders: " << (int)engine.getMissedEventCount() << std::endl;

    // Step 4: Patient sees OLED missed notification and presses CONFIRM on cell 1 to dismiss it
    std::cout << "\n--- Patient presses CONFIRM on any cell to clear missed notification ---" << std::endl;
    engine.handleCellButtonPress(0);
    std::cout << "Active Missed Reminders after dismissal: " << (int)engine.getMissedEventCount() << std::endl;

    // Step 5: Midnight rollover
    std::cout << "\n--- Advancing time to Midnight (23:59:50 -> 00:00:00) ---" << std::endl;
    timeProvider.setTime(23, 59, 50);
    engine.update();
    timeProvider.advanceSeconds(15);
    engine.update();

    std::cout << "\n========================================================" << std::endl;
    std::cout << " DEMO COMPLETED SUCCESSFULLY!" << std::endl;
    std::cout << "========================================================" << std::endl;

    return 0;
}
