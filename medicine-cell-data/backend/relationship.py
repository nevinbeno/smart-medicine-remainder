from .database import get_connection

# ============================
#   To get the Medicine specs as in table
# ============================
def get_cell_medicine(cell_id):
    connection = get_connection()

    try:
        cursor = connection.execute(
            """
            SELECT
                m.medicine_id,
                m.name,
                m.low_stock_threshold
            FROM cell c
            JOIN medicine m
                ON c.assigned_medicine_id = m.medicine_id
            WHERE c.cell_id = ?
            """,
            (cell_id,)
        )

        return cursor.fetchone()

    finally:
        connection.close()

# ============================
#   To get the schedules of a given medicine as in Shedules table
# ============================
def get_cell_schedules(cell_id):
    connection = get_connection()

    try:
        cursor = connection.execute(
            """
            SELECT
                s.schedule_id,
                s.hour,
                s.minute,
                s.dose_quantity
            FROM cell c
            JOIN schedule s
                ON c.assigned_medicine_id = s.medicine_id
            WHERE c.cell_id = ?
            ORDER BY s.hour, s.minute
            """,
            (cell_id,)
        )

        return cursor.fetchall()

    finally:
        connection.close()

# ============================
#   To get the whole details (Medicine detail + Schedule)
# ============================
def get_cell_details(cell_id):
    connection = get_connection()

    try:
        cell_cursor = connection.execute(
            """
            SELECT
                cell_id,
                assigned_medicine_id,
                current_stock,
                cell_state
            FROM cell
            WHERE cell_id = ?
            """,
            (cell_id,)
        )

        cell = cell_cursor.fetchone()

        if cell is None:
            return None

        medicine_cursor = connection.execute(
            """
            SELECT
                m.medicine_id,
                m.name,
                m.low_stock_threshold
            FROM cell c
            JOIN medicine m
                ON c.assigned_medicine_id = m.medicine_id
            WHERE c.cell_id = ?
            """,
            (cell_id,)
        )

        medicine = medicine_cursor.fetchone()

        schedule_cursor = connection.execute(
            """
            SELECT
                s.schedule_id,
                s.hour,
                s.minute,
                s.dose_quantity
            FROM cell c
            JOIN schedule s
                ON c.assigned_medicine_id = s.medicine_id
            WHERE c.cell_id = ?
            ORDER BY s.hour, s.minute
            """,
            (cell_id,)
        )

        schedules = schedule_cursor.fetchall()

        return {
            "cell": cell,
            "medicine": medicine,
            "schedules": schedules
        }

    finally:
        connection.close()