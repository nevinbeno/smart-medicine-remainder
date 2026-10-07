#include <unity.h>
#include "../../include/dose_manager.h"

void setUp() {}
void tearDown() {}

void test_due_confirmation_decrements_stock_once() {
    DoseManager manager(60);
    TEST_ASSERT_TRUE(manager.addCell(1, "Medicine A", 5, 2));
    TEST_ASSERT_TRUE(manager.addDose(10, 1, 1000, 2));

    manager.update(1000);
    TEST_ASSERT_EQUAL(static_cast<int>(DoseStatus::Due),
                      static_cast<int>(manager.getDose(10)->status));
    TEST_ASSERT_EQUAL(static_cast<int>(DoseResult::Confirmed),
                      static_cast<int>(manager.confirmCell(1, 1001)));
    TEST_ASSERT_EQUAL_UINT16(3, manager.getCell(1)->stock);
    TEST_ASSERT_EQUAL(static_cast<int>(DoseStatus::Confirmed),
                      static_cast<int>(manager.getDose(10)->status));
    TEST_ASSERT_EQUAL(static_cast<int>(DoseResult::NoDueDose),
                      static_cast<int>(manager.confirmCell(1, 1002)));
    TEST_ASSERT_EQUAL_UINT16(3, manager.getCell(1)->stock);
}

void test_missed_dose_ack_does_not_resolve_or_decrement_stock() {
    DoseManager manager(60);
    TEST_ASSERT_TRUE(manager.addCell(1, "Medicine A", 4, 1));
    TEST_ASSERT_TRUE(manager.addDose(11, 1, 1000, 1));

    manager.update(1060);
    TEST_ASSERT_TRUE(manager.hasMissedAlert());
    TEST_ASSERT_EQUAL_UINT32(1, manager.missedDoseCount());
    TEST_ASSERT_EQUAL(static_cast<int>(DoseResult::MissedDoseNotCounted),
                      static_cast<int>(manager.confirmCell(1, 1061)));
    TEST_ASSERT_FALSE(manager.hasMissedAlert());
    TEST_ASSERT_EQUAL_UINT32(1, manager.missedDoseCount());
    TEST_ASSERT_EQUAL_UINT16(4, manager.getCell(1)->stock);
    TEST_ASSERT_EQUAL(static_cast<int>(DoseStatus::Missed),
                      static_cast<int>(manager.getDose(11)->status));
}

void test_wrong_cell_cannot_confirm_due_dose() {
    DoseManager manager(60);
    TEST_ASSERT_TRUE(manager.addCell(1, "Medicine A", 3, 1));
    TEST_ASSERT_TRUE(manager.addCell(2, "Medicine B", 3, 1));
    TEST_ASSERT_TRUE(manager.addDose(12, 1, 1000));

    TEST_ASSERT_EQUAL(static_cast<int>(DoseResult::NoDueDose),
                      static_cast<int>(manager.confirmCell(2, 1001)));
    TEST_ASSERT_EQUAL(static_cast<int>(DoseStatus::Due),
                      static_cast<int>(manager.getDose(12)->status));
    TEST_ASSERT_EQUAL_UINT16(3, manager.getCell(1)->stock);
}

void test_multiple_due_doses_in_one_cell_need_separate_confirmations() {
    DoseManager manager(300);
    TEST_ASSERT_TRUE(manager.addCell(1, "Medicine A", 5, 3));
    TEST_ASSERT_TRUE(manager.addDose(13, 1, 1000));
    TEST_ASSERT_TRUE(manager.addDose(14, 1, 1000));

    TEST_ASSERT_EQUAL(static_cast<int>(DoseResult::Confirmed),
                      static_cast<int>(manager.confirmCell(1, 1001)));
    TEST_ASSERT_EQUAL_UINT16(4, manager.getCell(1)->stock);
    TEST_ASSERT_EQUAL(static_cast<int>(DoseStatus::Due),
                      static_cast<int>(manager.getDose(14)->status));
    TEST_ASSERT_EQUAL(static_cast<int>(DoseResult::ConfirmedLowStock),
                      static_cast<int>(manager.confirmCell(1, 1002)));
    TEST_ASSERT_EQUAL_UINT16(3, manager.getCell(1)->stock);
    TEST_ASSERT_TRUE(manager.getCell(1)->isLow());
}

void test_stock_cannot_underflow() {
    DoseManager manager(60);
    TEST_ASSERT_TRUE(manager.addCell(1, "Medicine A", 0, 0));
    TEST_ASSERT_TRUE(manager.addDose(15, 1, 1000, 1));

    TEST_ASSERT_EQUAL(static_cast<int>(DoseResult::ConfirmedStockEmpty),
                      static_cast<int>(manager.confirmCell(1, 1001)));
    TEST_ASSERT_EQUAL_UINT16(0, manager.getCell(1)->stock);
    TEST_ASSERT_EQUAL(static_cast<int>(DoseStatus::Confirmed),
                      static_cast<int>(manager.getDose(15)->status));
}

void test_restock_clears_low_stock_without_overflow() {
    DoseManager manager;
    TEST_ASSERT_TRUE(manager.addCell(1, "Medicine A", 1, 2));
    TEST_ASSERT_TRUE(manager.getCell(1)->isLow());
    TEST_ASSERT_TRUE(manager.restockCell(1, 2));
    TEST_ASSERT_EQUAL_UINT16(3, manager.getCell(1)->stock);
    TEST_ASSERT_FALSE(manager.getCell(1)->isLow());
    TEST_ASSERT_FALSE(manager.restockCell(1, 0));
    TEST_ASSERT_FALSE(manager.restockCell(1, UINT16_MAX));
    TEST_ASSERT_EQUAL_UINT16(3, manager.getCell(1)->stock);
}

void test_duplicate_ids_and_invalid_doses_are_rejected() {
    DoseManager manager;
    TEST_ASSERT_TRUE(manager.addCell(1, "Medicine A", 4, 1));
    TEST_ASSERT_FALSE(manager.addCell(1, "Duplicate", 4, 1));
    TEST_ASSERT_FALSE(manager.addDose(20, 2, 1000));
    TEST_ASSERT_FALSE(manager.addDose(20, 1, 1000, 0));
    TEST_ASSERT_TRUE(manager.addDose(20, 1, 1000));
    TEST_ASSERT_FALSE(manager.addDose(20, 1, 2000));
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_due_confirmation_decrements_stock_once);
    RUN_TEST(test_missed_dose_ack_does_not_resolve_or_decrement_stock);
    RUN_TEST(test_wrong_cell_cannot_confirm_due_dose);
    RUN_TEST(test_multiple_due_doses_in_one_cell_need_separate_confirmations);
    RUN_TEST(test_stock_cannot_underflow);
    RUN_TEST(test_restock_clears_low_stock_without_overflow);
    RUN_TEST(test_duplicate_ids_and_invalid_doses_are_rejected);
    return UNITY_END();
}