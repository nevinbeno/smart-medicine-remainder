import json
from pathlib import Path

from .database import get_connection

# Project root
PROJECT_ROOT = Path(__file__).resolve().parent.parent

# Configuration output location
CONFIG_DIR = PROJECT_ROOT / "config"
CONFIG_FILE = CONFIG_DIR / "configuration.json"


def generate_configuration():
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

        cells = cursor.fetchall()

        configuration = {
            "cellCount": len(cells),
            "cells": []
        }

        for cell_id, medicine_id, current_stock, cell_state in cells:

            cell_data = {
                "cellId": cell_id,
                "state": cell_state,
                "currentStock": current_stock,
                "medicine": None
            }

            # Empty cell or cell without an assigned medicine
            if medicine_id is None:
                configuration["cells"].append(cell_data)
                continue

            # Get medicine information
            medicine_cursor = connection.execute(
                """
                SELECT
                    medicine_id,
                    name,
                    low_stock_threshold
                FROM medicine
                WHERE medicine_id = ?
                """,
                (medicine_id,)
            )

            medicine = medicine_cursor.fetchone()

            if medicine is None:
                configuration["cells"].append(cell_data)
                continue

            medicine_id, name, low_stock_threshold = medicine

            # Get schedules for this medicine
            schedule_cursor = connection.execute(
                """
                SELECT
                    hour,
                    minute,
                    dose_quantity
                FROM schedule
                WHERE medicine_id = ?
                ORDER BY hour, minute
                """,
                (medicine_id,)
            )

            schedules = schedule_cursor.fetchall()

            schedule_data = []

            for hour, minute, dose_quantity in schedules:
                schedule_data.append(
                    {
                        "hour": hour,
                        "minute": minute,
                        "doseQuantity": dose_quantity
                    }
                )

            cell_data["medicine"] = {
                "medicineId": medicine_id,
                "name": name,
                "lowStockThreshold": low_stock_threshold,
                "schedules": schedule_data
            }

            configuration["cells"].append(cell_data)

        # Create config/ if it doesn't exist
        CONFIG_DIR.mkdir(parents=True, exist_ok=True)

        # Write configuration.json
        with CONFIG_FILE.open("w", encoding="utf-8") as file:
            json.dump(configuration, file, indent=4)

        print(f"Configuration generated: {CONFIG_FILE}")

    finally:
        connection.close()


if __name__ == "__main__":
    generate_configuration()