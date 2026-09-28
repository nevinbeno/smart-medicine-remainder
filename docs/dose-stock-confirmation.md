# Dose, Stock & Confirmation Management

This module is designed to be the software contract for role 4's cell/event
engine and role 3's button/LED handling. It is hardware-independent and lives in
`include/dose_manager.h`.

## Behavior

- Each cell has a stable cell ID, medicine name, remaining unit count, and low
  stock threshold. Stock is low when `stock <= lowStockThreshold` (yellow LED).
- Refills use `restockCell(cellId, units)`. Invalid/zero refills and values that
  would overflow the stock counter are rejected without changing inventory.
- Each scheduled dose is a distinct occurrence with a unique dose ID, cell ID,
  RTC Unix timestamp, and number of units.
- `update(now)` marks an occurrence Due at its scheduled time. It becomes Missed
  when the configured grace interval has elapsed (15 minutes by default).
- A cell confirmation acknowledges pending missed-dose notifications globally,
  then confirms at most the oldest Due occurrence in that same cell. Confirming
  a due dose decrements stock once, marks the occurrence Confirmed, and drives
  the cell's green confirmation indication for one second in the hardware layer.
- Pressing a cell button for a Missed occurrence only dismisses the OLED alert.
  The occurrence stays Missed, stock is not decremented, and the missed-dose
  history remains queryable. A confirmation from the wrong cell cannot confirm
  another cell's dose.
- If a confirmation is received when the inventory is already below the dose
  quantity, the dose is still marked Confirmed (the user reports taking it),
  stock is clamped at zero, and `ConfirmedStockEmpty` signals an inventory
  discrepancy. Stock never underflows.
- If multiple doses are due in one cell, each button press confirms only one;
  the cell remains due until its other occurrences are handled. Simultaneous
  doses in different cells can be processed independently.
- Missed notifications are an in-memory latch; acknowledging one does not
  delete or resolve missed occurrences. New missed occurrences raise the latch
  again.

## Integration

1. Create a `DoseManager` with the chosen missed grace interval in seconds.
2. Register each compartment using `addCell(cellId, name, stock, lowThreshold)`.
3. Add each scheduled occurrence with `addDose(doseId, cellId, unixTime, units)`.
4. Call `update(rtc.now().unixtime())` regularly. Drive red for cells with Due
   occurrences, yellow for low inventory, and show a missed alert while
   `hasMissedAlert()` is true. The UI can query `getDose()` and `getCell()`.
5. On any cell button press, call `confirmCell(cellId, now)`. Interpret the
   returned `DoseResult` for OLED feedback. `Confirmed`, `ConfirmedLowStock`,
   and `ConfirmedStockEmpty` all represent a consumed/confirmed due occurrence;
   `MissedDoseNotCounted` and `NoDueDose` must not change stock.

`DoseManager` does not persist schedules, stock, or history across resets and
does not physically sense medicine removal. Until non-volatile storage and a
validated sensor are added, the user's confirmation is the source of truth.
Never treat a missed-dose alert acknowledgment as proof that medicine was taken.

## Validation

Automated unit tests are in `test/test_dose_manager/test_main.cpp`. Run them
with PlatformIO's native environment (`pio test -e native`). Hardware integration
tests should also verify LED/UI behavior and button debounce with the final
Wokwi diagram or physical device.