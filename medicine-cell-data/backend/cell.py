from .database import get_connection


def add_cell(cell_id):
    connection = get_connection()

    try:
        connection.execute(
            """
            INSERT INTO cell (cell_id)
            VALUES (?)
            """,
            (cell_id,)
        )

        connection.commit()

    finally:
        connection.close()


def get_cell(cell_id):
    connection = get_connection()

    try:
        cursor = connection.execute(
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

        return cursor.fetchone()

    finally:
        connection.close()


def list_cells():
    connection = get_connection()

    try:
        cursor = connection.execute(
            """
            SELECT
                cell_id,
                assigned_medicine_id,
                current_stock,
                cell_state
            FROM cell
            ORDER BY cell_id
            """
        )

        return cursor.fetchall()

    finally:
        connection.close()


def assign_medicine(cell_id, medicine_id):
    connection = get_connection()

    try:
        cursor = connection.execute(
            """
            UPDATE cell
            SET assigned_medicine_id = ?,
                cell_state = 'OCCUPIED'
            WHERE cell_id = ?
            AND cell_state != 'DISABLED'
            """,
            (medicine_id, cell_id)
        )

        connection.commit()

        return cursor.rowcount > 0

    finally:
        connection.close()


def remove_medicine(cell_id):
    connection = get_connection()

    try:
        cursor = connection.execute(
            """
            UPDATE cell
            SET assigned_medicine_id = NULL,
                current_stock = 0,
                cell_state = 'EMPTY'
            WHERE cell_id = ?
            AND cell_state != 'DISABLED'
            """,
            (cell_id,)
        )

        connection.commit()

        return cursor.rowcount > 0

    finally:
        connection.close()


def set_stock(cell_id, stock):
    connection = get_connection()

    try:
        cursor = connection.execute(
            """
            UPDATE cell
            SET current_stock = ?
            WHERE cell_id = ?
            AND cell_state != 'DISABLED'
            """,
            (stock, cell_id)
        )

        connection.commit()

        return cursor.rowcount > 0

    finally:
        connection.close()


def disable_cell(cell_id):
    connection = get_connection()

    try:
        cursor = connection.execute(
            """
            UPDATE cell
            SET cell_state = 'DISABLED'
            WHERE cell_id = ?
            AND cell_state != 'DISABLED'
            """,
            (cell_id,)
        )

        connection.commit()

        return cursor.rowcount > 0

    finally:
        connection.close()


def enable_cell(cell_id):
    connection = get_connection()

    try:
        cursor = connection.execute(
            """
            UPDATE cell
            SET cell_state =
                CASE
                    WHEN assigned_medicine_id IS NULL
                    THEN 'EMPTY'
                    ELSE 'OCCUPIED'
                END
            WHERE cell_id = ?
            AND cell_state = 'DISABLED'
            """,
            (cell_id,)
        )

        connection.commit()

        return cursor.rowcount > 0

    finally:
        connection.close()