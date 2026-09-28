# Header Files
This directory contains the header files that define the core data structures used by the Medicine and Cell Data module.

## `medicine.hpp`

Defines the `Medicine` data structure.

A medicine contains:

- Medicine ID
- Medicine name
- Low-stock threshold
- List of scheduled doses
- Number of configured schedules

It describes the medicine and its configuration, but does not store
physical stock.

---

## `schedule.hpp`

Defines the `ScheduleEntry` data structure.

Each schedule entry represents one scheduled dose and contains:

- Scheduled hour
- Scheduled minute
- Dose quantity

A medicine can contain multiple schedule entries.

Example:

```text
08:00 → 1 dose
14:00 → 1 dose
20:00 → 2 doses
```

## `cell.hpp`
Defines the `Cell` data structure and `CellState`. 

A cell represents one physical medicine storage compartment.

It contains:
- Cell ID
- Assigned medicine ID
- Current stock
- Cell state

 ### Cell States
```txt
EMPTY 
OCCUPIED 
DISABLED 
```

   ### Description: 
   - EMPTY: no medicine is currently assigne
   - OCCUPIED: cell is assigned to a medicine and is available for use
   - DISABLED: cell is intentionally unavailable

These describe the availability/assignment state of the physical cell.

Runtime conditions such as DOSE_DUE, MISSED, or LOW_STOCK are handled by other modules and are not part of CellState.