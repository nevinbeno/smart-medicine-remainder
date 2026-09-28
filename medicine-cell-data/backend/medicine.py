from .database import get_connection


def add_medicine(medicine_id, name, low_stock_threshold):
    connection = get_connection()

    try:
        connection.execute(
            """
            INSERT INTO medicine
                (medicine_id, name, low_stock_threshold)
            VALUES (?, ?, ?)
            """,
            (medicine_id, name, low_stock_threshold)
        )

        connection.commit()

    finally:
        connection.close()


def get_medicine(medicine_id):
    connection = get_connection()

    try:
        cursor = connection.execute(
            """
            SELECT medicine_id, name, low_stock_threshold
            FROM medicine
            WHERE medicine_id = ?
            """,
            (medicine_id,)
        )

        return cursor.fetchone()

    finally:
        connection.close()


def list_medicines():
    connection = get_connection()

    try:
        cursor = connection.execute(
            """
            SELECT medicine_id, name, low_stock_threshold
            FROM medicine
            ORDER BY medicine_id
            """
        )

        return cursor.fetchall()

    finally:
        connection.close()


def update_medicine(medicine_id, name, low_stock_threshold):
    connection = get_connection()

    try:
        cursor = connection.execute(
            """
            UPDATE medicine
            SET name = ?, low_stock_threshold = ?
            WHERE medicine_id = ?
            """,
            (name, low_stock_threshold, medicine_id)
        )

        connection.commit()

        return cursor.rowcount > 0

    finally:
        connection.close()


def remove_medicine(medicine_id):
    connection = get_connection()

    try:
        cursor = connection.execute(
            """
            DELETE FROM medicine
            WHERE medicine_id = ?
            """,
            (medicine_id,)
        )

        connection.commit()

        return cursor.rowcount > 0

    finally:
        connection.close()