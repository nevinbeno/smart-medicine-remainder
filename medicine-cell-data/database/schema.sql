PRAGMA foreign_keys = ON;

-- =========================================================
-- MEDICINE
-- =========================================================

CREATE TABLE IF NOT EXISTS medicine (
    medicine_id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    low_stock_threshold INTEGER NOT NULL
);

-- =========================================================
-- SCHEDULE
-- =========================================================

CREATE TABLE IF NOT EXISTS schedule (
    schedule_id INTEGER PRIMARY KEY AUTOINCREMENT,
    medicine_id TEXT NOT NULL,
    hour INTEGER NOT NULL,
    minute INTEGER NOT NULL,
    dose_quantity INTEGER NOT NULL,

    FOREIGN KEY (medicine_id)
        REFERENCES medicine(medicine_id)
        ON DELETE CASCADE,

    CHECK (hour >= 0 AND hour <= 23),
    CHECK (minute >= 0 AND minute <= 59),
    CHECK (dose_quantity > 0)
);

-- =========================================================
-- CELL
-- =========================================================

CREATE TABLE IF NOT EXISTS cell (
    cell_id TEXT PRIMARY KEY,
    assigned_medicine_id TEXT,
    current_stock INTEGER NOT NULL DEFAULT 0,
    cell_state TEXT NOT NULL DEFAULT 'EMPTY',

    FOREIGN KEY (assigned_medicine_id)
        REFERENCES medicine(medicine_id),

    CHECK (cell_state IN ('EMPTY', 'OCCUPIED', 'DISABLED')),
    CHECK (current_stock >= 0)
);