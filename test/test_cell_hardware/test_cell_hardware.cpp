#include <Arduino.h>
#include <unity.h>
#include "CellHardwareEvents.h"

// Test mock tracking variables
static uint8_t g_lastConfirmedCell = 255;
static uint8_t g_confirmCallCount = 0;

static void testCallback(uint8_t cellId) {
    g_lastConfirmedCell = cellId;
    g_confirmCallCount++;
}

void setUp(void) {
    g_lastConfirmedCell = 255;
    g_confirmCallCount = 0;
}

void tearDown(void) {
}

void test_initial_4cell_state(void) {
    CellHardwareController controller(4);
    controller.begin();

    TEST_ASSERT_EQUAL(4, controller.getCellCount());
    for (uint8_t i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL(CELL_STATE_IDLE, controller.getCellState(i));
        CellHardware* cell = controller.getCell(i);
        TEST_ASSERT_NOT_NULL(cell);
        TEST_ASSERT_FALSE(cell->getServo().isOpen());
        TEST_ASSERT_EQUAL(0, cell->getServo().getCurrentAngle());
    }
    TEST_ASSERT_EQUAL(0, controller.getActiveAlertCount());
    TEST_ASSERT_FALSE(controller.getBuzzer().isActive());
}

void test_individual_servos_open_only_for_due_cell(void) {
    CellHardwareController controller(4);
    controller.begin();

    // Trigger Cell 1 (id 0) DUE
    bool ok = controller.triggerMedicineDue(0);
    TEST_ASSERT_TRUE(ok);

    // Verify Cell 1: Alerting, Servo 1 OPEN (90 deg)
    TEST_ASSERT_EQUAL(CELL_STATE_ALERTING, controller.getCellState(0));
    TEST_ASSERT_TRUE(controller.getCell(0)->getServo().isOpen());
    TEST_ASSERT_EQUAL(90, controller.getCell(0)->getServo().getCurrentAngle());

    // CRITICAL: Servos 2, 3, 4 MUST REMAIN CLOSED (0 deg)!
    for (uint8_t i = 1; i < 4; i++) {
        TEST_ASSERT_EQUAL(CELL_STATE_IDLE, controller.getCellState(i));
        TEST_ASSERT_FALSE(controller.getCell(i)->getServo().isOpen());
        TEST_ASSERT_EQUAL(0, controller.getCell(i)->getServo().getCurrentAngle());
    }

    // Now trigger Cell 3 (id 2) DUE
    controller.triggerMedicineDue(2);
    TEST_ASSERT_TRUE(controller.getCell(2)->getServo().isOpen());
    TEST_ASSERT_EQUAL(90, controller.getCell(2)->getServo().getCurrentAngle());

    // Cell 2 (id 1) and Cell 4 (id 3) must STILL be closed!
    TEST_ASSERT_FALSE(controller.getCell(1)->getServo().isOpen());
    TEST_ASSERT_FALSE(controller.getCell(3)->getServo().isOpen());
}

void test_confirm_closes_only_that_cells_servo(void) {
    CellHardwareController controller(4);
    controller.setConfirmationCallback(testCallback);
    controller.begin();

    // Make both Cell 1 (id 0) and Cell 2 (id 1) due
    controller.triggerMedicineDue(0);
    controller.triggerMedicineDue(1);

    TEST_ASSERT_TRUE(controller.getCell(0)->getServo().isOpen());
    TEST_ASSERT_TRUE(controller.getCell(1)->getServo().isOpen());

    // Confirm Cell 1 ONLY:
    controller.triggerDoseConfirmed(0);

    // Servo 1 must CLOSE (0 deg)
    TEST_ASSERT_FALSE(controller.getCell(0)->getServo().isOpen());
    TEST_ASSERT_EQUAL(0, controller.getCell(0)->getServo().getCurrentAngle());
    TEST_ASSERT_EQUAL(CELL_STATE_CONFIRMED, controller.getCellState(0));

    // CRITICAL: Servo 2 MUST REMAIN OPEN (90 deg)!
    TEST_ASSERT_TRUE(controller.getCell(1)->getServo().isOpen());
    TEST_ASSERT_EQUAL(90, controller.getCell(1)->getServo().getCurrentAngle());
    TEST_ASSERT_EQUAL(CELL_STATE_ALERTING, controller.getCellState(1));
}

void test_common_buzzer_arbitration_across_all_4_cells(void) {
    CellHardwareController controller(4);
    controller.setConfirmationCallback(testCallback);
    controller.begin();

    // Trigger all 4 cells due simultaneously
    controller.triggerMedicineDue(0);
    controller.triggerMedicineDue(1);
    controller.triggerMedicineDue(2);
    controller.triggerMedicineDue(3);

    TEST_ASSERT_EQUAL(4, controller.getActiveAlertCount());
    TEST_ASSERT_TRUE(controller.getBuzzer().isActive());

    // Confirm Cell 1: Count becomes 3 -> Buzzer MUST stay active
    controller.triggerDoseConfirmed(0);
    TEST_ASSERT_EQUAL(3, controller.getActiveAlertCount());
    TEST_ASSERT_TRUE(controller.getBuzzer().isActive());

    // Confirm Cell 2: Count becomes 2 -> Buzzer MUST stay active
    controller.triggerDoseConfirmed(1);
    TEST_ASSERT_EQUAL(2, controller.getActiveAlertCount());
    TEST_ASSERT_TRUE(controller.getBuzzer().isActive());

    // Confirm Cell 3: Count becomes 1 -> Buzzer MUST stay active
    controller.triggerDoseConfirmed(2);
    TEST_ASSERT_EQUAL(1, controller.getActiveAlertCount());
    TEST_ASSERT_TRUE(controller.getBuzzer().isActive());

    // Confirm Cell 4: Count becomes 0 -> Buzzer MUST silence immediately
    controller.triggerDoseConfirmed(3);
    TEST_ASSERT_EQUAL(0, controller.getActiveAlertCount());
    TEST_ASSERT_FALSE(controller.getBuzzer().isActive());
}

void test_missed_dose_warning_per_cell(void) {
    CellHardwareController controller(4);
    controller.begin();

    controller.triggerMedicineDue(1); // Cell 2 due
    TEST_ASSERT_TRUE(controller.getCell(1)->getServo().isOpen());

    // Transition Cell 2 to MISSED_DOSE
    controller.triggerMissedDose(1);
    TEST_ASSERT_EQUAL(CELL_STATE_WARNING, controller.getCellState(1));
    TEST_ASSERT_TRUE(controller.isCellWarning(1));
    TEST_ASSERT_FALSE(controller.getCell(1)->getServo().isOpen()); // Closes servo
}

void test_reset_functionality(void) {
    CellHardwareController controller(4);
    controller.begin();

    controller.triggerMedicineDue(3); // Cell 4 due
    TEST_ASSERT_EQUAL(CELL_STATE_ALERTING, controller.getCellState(3));

    controller.triggerReset(3);
    TEST_ASSERT_EQUAL(CELL_STATE_IDLE, controller.getCellState(3));
    TEST_ASSERT_FALSE(controller.getCell(3)->getServo().isOpen());
    TEST_ASSERT_EQUAL(0, controller.getActiveAlertCount());
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_initial_4cell_state);
    RUN_TEST(test_individual_servos_open_only_for_due_cell);
    RUN_TEST(test_confirm_closes_only_that_cells_servo);
    RUN_TEST(test_common_buzzer_arbitration_across_all_4_cells);
    RUN_TEST(test_missed_dose_warning_per_cell);
    RUN_TEST(test_reset_functionality);
    UNITY_END();
}

void loop() {
}
