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

void test_dynamic_cell_count_initialization(void) {
    CellHardwareController controller(2);
    controller.begin();

    TEST_ASSERT_EQUAL(2, controller.getCellCount());
    TEST_ASSERT_NOT_NULL(controller.getCell(0));
    TEST_ASSERT_NOT_NULL(controller.getCell(1));
    TEST_ASSERT_NULL(controller.getCell(2));
    TEST_ASSERT_NULL(controller.getCell(3));

    // Active cell 0 responds to trigger
    TEST_ASSERT_TRUE(controller.triggerMedicineDue(0));
    TEST_ASSERT_EQUAL(CELL_STATE_ALERTING, controller.getCellState(0));
    TEST_ASSERT_TRUE(controller.getCell(0)->getServo().isOpen());

    // Inactive cell 2 rejects trigger
    TEST_ASSERT_FALSE(controller.triggerMedicineDue(2));
    TEST_ASSERT_EQUAL(CELL_STATE_IDLE, controller.getCellState(2));
}

void test_dynamic_setCellCount_expansion(void) {
    CellHardwareController controller(2);
    controller.begin();

    TEST_ASSERT_EQUAL(2, controller.getCellCount());
    TEST_ASSERT_FALSE(controller.triggerMedicineDue(3));

    // Dynamically expand to 4 cells
    TEST_ASSERT_TRUE(controller.setCellCount(4));
    TEST_ASSERT_EQUAL(4, controller.getCellCount());
    TEST_ASSERT_NOT_NULL(controller.getCell(3));

    // Cell 3 is now active and responds to trigger
    TEST_ASSERT_TRUE(controller.triggerMedicineDue(3));
    TEST_ASSERT_EQUAL(CELL_STATE_ALERTING, controller.getCellState(3));
    TEST_ASSERT_TRUE(controller.getCell(3)->getServo().isOpen());
    TEST_ASSERT_TRUE(controller.getBuzzer().isActive());

    // Confirm cell 3
    TEST_ASSERT_TRUE(controller.triggerDoseConfirmed(3));
    TEST_ASSERT_FALSE(controller.getCell(3)->getServo().isOpen());
    TEST_ASSERT_FALSE(controller.getBuzzer().isActive());
}

void test_dynamic_setCellCount_reduction_and_cleanup(void) {
    CellHardwareController controller(4);
    controller.begin();

    // Trigger Cell 0 and Cell 2 DUE
    controller.triggerMedicineDue(0);
    controller.triggerMedicineDue(2);
    TEST_ASSERT_EQUAL(2, controller.getActiveAlertCount());
    TEST_ASSERT_TRUE(controller.getBuzzer().isActive());

    // Reduce active cells to 2 (Cells 0 and 1 remain, 2 and 3 deactivated)
    TEST_ASSERT_TRUE(controller.setCellCount(2));
    TEST_ASSERT_EQUAL(2, controller.getCellCount());

    // Cell 2 was deactivated and must be cleaned up / reset
    TEST_ASSERT_EQUAL(CELL_STATE_IDLE, controller.getCellState(2));
    TEST_ASSERT_NULL(controller.getCell(2));

    // Only Cell 0 remains alerting
    TEST_ASSERT_EQUAL(1, controller.getActiveAlertCount());
    TEST_ASSERT_TRUE(controller.getBuzzer().isActive());

    // Confirm Cell 0 -> all alerts clear, buzzer silences
    controller.triggerDoseConfirmed(0);
    TEST_ASSERT_EQUAL(0, controller.getActiveAlertCount());
    TEST_ASSERT_FALSE(controller.getBuzzer().isActive());
}

void test_dynamic_cell_count_bounds_validation(void) {
    CellHardwareController controller(3);
    controller.begin();

    TEST_ASSERT_EQUAL(3, controller.getCellCount());

    // Invalid bounds should be rejected and leave count unchanged
    TEST_ASSERT_FALSE(controller.setCellCount(0));
    TEST_ASSERT_EQUAL(3, controller.getCellCount());

    TEST_ASSERT_FALSE(controller.setCellCount(65));
    TEST_ASSERT_EQUAL(3, controller.getCellCount());

    // Same count should return true safely
    TEST_ASSERT_TRUE(controller.setCellCount(3));
    TEST_ASSERT_EQUAL(3, controller.getCellCount());
}

void test_matrix_grid_2x2_4x4_8x8_scaling(void) {
    CellHardwareController controller(4);
    controller.begin();

    // Default 2x2 grid
    TEST_ASSERT_EQUAL(2, controller.getGridRows());
    TEST_ASSERT_EQUAL(2, controller.getGridCols());
    TEST_ASSERT_EQUAL(4, controller.getCellCount());

    // Scale to 4x4 grid (16 cells)
    TEST_ASSERT_TRUE(controller.setGrid(4, 4));
    TEST_ASSERT_EQUAL(4, controller.getGridRows());
    TEST_ASSERT_EQUAL(4, controller.getGridCols());
    TEST_ASSERT_EQUAL(16, controller.getCellCount());

    // Scale to 8x8 grid (64 cells)
    TEST_ASSERT_TRUE(controller.setGrid(8, 8));
    TEST_ASSERT_EQUAL(8, controller.getGridRows());
    TEST_ASSERT_EQUAL(8, controller.getGridCols());
    TEST_ASSERT_EQUAL(64, controller.getCellCount());

    // Scale to 2x4 grid (8 cells)
    TEST_ASSERT_TRUE(controller.setGrid(2, 4));
    TEST_ASSERT_EQUAL(2, controller.getGridRows());
    TEST_ASSERT_EQUAL(4, controller.getGridCols());
    TEST_ASSERT_EQUAL(8, controller.getCellCount());

    // Bounds rejection: 9x8 > 64 cells, or 0 dimensions
    TEST_ASSERT_FALSE(controller.setGrid(9, 8));
    TEST_ASSERT_FALSE(controller.setGrid(0, 4));
    TEST_ASSERT_FALSE(controller.setGrid(4, 0));
    TEST_ASSERT_EQUAL(8, controller.getCellCount());
}

void test_matrix_grid_coordinate_translation_and_triggers(void) {
    CellHardwareController controller(16);
    controller.setGrid(4, 4);
    controller.begin();

    // Coordinate mapping tests (1-indexed row, col -> 0-indexed linear)
    TEST_ASSERT_EQUAL(0, controller.gridCoordsToIndex(1, 1));
    TEST_ASSERT_EQUAL(6, controller.gridCoordsToIndex(2, 3)); // (2-1)*4 + (3-1) = 6
    TEST_ASSERT_EQUAL(15, controller.gridCoordsToIndex(4, 4));
    TEST_ASSERT_EQUAL(-1, controller.gridCoordsToIndex(5, 1)); // out of bounds
    TEST_ASSERT_EQUAL(-1, controller.gridCoordsToIndex(1, 5));

    uint8_t r = 0, c = 0;
    TEST_ASSERT_TRUE(controller.indexToGridCoords(6, r, c));
    TEST_ASSERT_EQUAL(2, r);
    TEST_ASSERT_EQUAL(3, c);

    // Matrix grid trigger tests
    TEST_ASSERT_TRUE(controller.triggerMedicineDueGrid(2, 3));
    TEST_ASSERT_EQUAL(CELL_STATE_ALERTING, controller.getCellStateGrid(2, 3));
    TEST_ASSERT_EQUAL(CELL_STATE_ALERTING, controller.getCellState(6));
    TEST_ASSERT_TRUE(controller.getCell(6)->getServo().isOpen());

    // Other cells must remain closed
    TEST_ASSERT_EQUAL(CELL_STATE_IDLE, controller.getCellStateGrid(1, 1));
    TEST_ASSERT_FALSE(controller.getCell(0)->getServo().isOpen());

    // Confirm via matrix grid
    TEST_ASSERT_TRUE(controller.triggerDoseConfirmedGrid(2, 3));
    TEST_ASSERT_EQUAL(CELL_STATE_CONFIRMED, controller.getCellStateGrid(2, 3));
    TEST_ASSERT_FALSE(controller.getCell(6)->getServo().isOpen());

    // Out of bounds trigger returns false
    TEST_ASSERT_FALSE(controller.triggerMedicineDueGrid(5, 5));
}

void test_matrix_grid_8x8_buzzer_arbitration(void) {
    CellHardwareController controller;
    controller.setGrid(8, 8); // 64 cells!
    controller.begin();

    TEST_ASSERT_EQUAL(64, controller.getCellCount());

    // Trigger opposite corners of 8x8 matrix: (1,1) -> Cell 0, (8,8) -> Cell 63
    TEST_ASSERT_TRUE(controller.triggerMedicineDueGrid(1, 1));
    TEST_ASSERT_TRUE(controller.triggerMedicineDueGrid(8, 8));

    TEST_ASSERT_EQUAL(2, controller.getActiveAlertCount());
    TEST_ASSERT_TRUE(controller.getBuzzer().isActive());

    // Confirm (1,1): alert count drops to 1, buzzer must remain active!
    TEST_ASSERT_TRUE(controller.triggerDoseConfirmedGrid(1, 1));
    TEST_ASSERT_EQUAL(1, controller.getActiveAlertCount());
    TEST_ASSERT_TRUE(controller.getBuzzer().isActive());

    // Confirm (8,8): alert count drops to 0, buzzer silences immediately!
    TEST_ASSERT_TRUE(controller.triggerDoseConfirmedGrid(8, 8));
    TEST_ASSERT_EQUAL(0, controller.getActiveAlertCount());
    TEST_ASSERT_FALSE(controller.getBuzzer().isActive());
}

void test_4x4_single_compartment_complete_isolation(void) {
    CellHardwareController controller(16);
    controller.setGrid(4, 4);
    controller.begin();

    TEST_ASSERT_EQUAL(16, controller.getCellCount());

    // 1. Verify that triggering ANY single cell in a 4x4 grid leaves all other 15 cells strictly IDLE
    for (uint8_t target = 0; target < 16; target++) {
        // Trigger target cell
        TEST_ASSERT_TRUE(controller.triggerMedicineDue(target));
        TEST_ASSERT_EQUAL(CELL_STATE_ALERTING, controller.getCellState(target));
        TEST_ASSERT_TRUE(controller.getCell(target)->getServo().isOpen());

        // Verify all other 15 cells are strictly IDLE and closed
        for (uint8_t other = 0; target < 16 && other < 16; other++) {
            if (other != target) {
                TEST_ASSERT_EQUAL(CELL_STATE_IDLE, controller.getCellState(other));
                TEST_ASSERT_FALSE(controller.getCell(other)->getServo().isOpen());
            }
        }

        // Confirm target cell: closes its servo, other 15 cells remain untouched
        TEST_ASSERT_TRUE(controller.triggerDoseConfirmed(target));
        TEST_ASSERT_EQUAL(CELL_STATE_CONFIRMED, controller.getCellState(target));
        TEST_ASSERT_FALSE(controller.getCell(target)->getServo().isOpen());

        for (uint8_t other = 0; other < 16; other++) {
            if (other != target) {
                TEST_ASSERT_EQUAL(CELL_STATE_IDLE, controller.getCellState(other));
            }
        }

        // Reset target back to IDLE before testing next cell
        TEST_ASSERT_TRUE(controller.triggerReset(target));
        TEST_ASSERT_EQUAL(CELL_STATE_IDLE, controller.getCellState(target));
    }

    // 2. Simultaneous independent alerts across distinct matrix coordinates (Row 2, Col 2) and (Row 4, Col 3)
    // Row 2, Col 2 -> index 5; Row 4, Col 3 -> index 14
    TEST_ASSERT_TRUE(controller.triggerMedicineDueGrid(2, 2));
    TEST_ASSERT_TRUE(controller.triggerMedicineDueGrid(4, 3));

    TEST_ASSERT_EQUAL(CELL_STATE_ALERTING, controller.getCellStateGrid(2, 2));
    TEST_ASSERT_EQUAL(CELL_STATE_ALERTING, controller.getCellStateGrid(4, 3));
    TEST_ASSERT_TRUE(controller.getCell(5)->getServo().isOpen());
    TEST_ASSERT_TRUE(controller.getCell(14)->getServo().isOpen());

    // Confirm ONLY Row 2, Col 2 -> Row 4, Col 3 MUST remain open and alerting
    TEST_ASSERT_TRUE(controller.triggerDoseConfirmedGrid(2, 2));
    TEST_ASSERT_EQUAL(CELL_STATE_CONFIRMED, controller.getCellStateGrid(2, 2));
    TEST_ASSERT_FALSE(controller.getCell(5)->getServo().isOpen());

    TEST_ASSERT_EQUAL(CELL_STATE_ALERTING, controller.getCellStateGrid(4, 3));
    TEST_ASSERT_TRUE(controller.getCell(14)->getServo().isOpen());

    // Finally confirm Row 4, Col 3
    TEST_ASSERT_TRUE(controller.triggerDoseConfirmedGrid(4, 3));
    TEST_ASSERT_EQUAL(CELL_STATE_CONFIRMED, controller.getCellStateGrid(4, 3));
    TEST_ASSERT_FALSE(controller.getCell(14)->getServo().isOpen());
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
    RUN_TEST(test_dynamic_cell_count_initialization);
    RUN_TEST(test_dynamic_setCellCount_expansion);
    RUN_TEST(test_dynamic_setCellCount_reduction_and_cleanup);
    RUN_TEST(test_dynamic_cell_count_bounds_validation);
    RUN_TEST(test_matrix_grid_2x2_4x4_8x8_scaling);
    RUN_TEST(test_matrix_grid_coordinate_translation_and_triggers);
    RUN_TEST(test_matrix_grid_8x8_buzzer_arbitration);
    RUN_TEST(test_4x4_single_compartment_complete_isolation);
    UNITY_END();
}

void loop() {
}
