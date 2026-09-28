from .database import get_connection


def add_schedule(medicine_id, hour, minute, dose_quantity):
    connection = get_connection()

    try:
        connection.execute(
            """
            INSERT INTO schedule
                (medicine_id, hour, minute, dose_quantity)
            VALUES (?, ?, ?, ?)
            """,
            (medicine_id, hour, minute, dose_quantity)
        )

        connection.commit()

    finally:
        connection.close()


def get_schedule(schedule_id):
    connection = get_connection()

    try:
        cursor = connection.execute(
            """
            SELECT schedule_id, medicine_id, hour, minute, dose_quantity
            FROM schedule
            WHERE schedule_id = ?
            """,
            (schedule_id,)
        )

        return cursor.fetchone()

    finally:
        connection.close()


def get_medicine_schedules(medicine_id):
    connection = get_connection()

    try:
        cursor = connection.execute(
            """
            SELECT schedule_id, medicine_id, hour, minute, dose_quantity
            FROM schedule
            WHERE medicine_id = ?
            ORDER BY hour, minute
            """,
            (medicine_id,)
        )

        return cursor.fetchall()

    finally:
        connection.close()


def update_schedule(schedule_id, hour, minute, dose_quantity):
    connection = get_connection()

    try:
        cursor = connection.execute(
            """
            UPDATE schedule
            SET hour = ?, minute = ?, dose_quantity = ?
            WHERE schedule_id = ?
            """,
            (hour, minute, dose_quantity, schedule_id)
        )

        connection.commit()

        return cursor.rowcount > 0

    finally:
        connection.close()


def remove_schedule(schedule_id):
    connection = get_connection()

    try:
        cursor = connection.execute(
            """
            DELETE FROM schedule
            WHERE schedule_id = ?
            """,
            (schedule_id,)
        )

        connection.commit()

        return cursor.rowcount > 0

    finally:
        connection.close()