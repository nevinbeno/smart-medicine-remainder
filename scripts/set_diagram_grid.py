#!/usr/bin/env python3
"""
scripts/set_diagram_grid.py
Configures Wokwi's diagram.json layout dynamically based on the requested grid dimensions.

Supports 1x1 (1 cell), 1x2 (2 cells), 2x2 (4 cells), 4x4 (16 cells), 8x8 (64 cells).
Arranges all requested compartments (Servo, RGB LED, Button) in an exact matrix on the Wokwi canvas.

Usage:
    python3 scripts/set_diagram_grid.py [1x1 | 1x2 | 2x1 | 2x2 | 4x4 | 8x8]
    python3 scripts/set_diagram_grid.py --config grid_config.json

Author: Pavithra (Role 4: Cell Hardware & Alert Control)
"""

import json
import os
import sys
import argparse

CONFIG_FILE = "grid_config.json"
DIAGRAM_FILE = "diagram.json"

# Physical GPIO assignments on ESP32 DevKit V1 for direct cells (Cells 1-4)
CELL_PINS = [
    {
        "id": 1,
        "name": "Cell 1",
        "red_pin": "esp:D25",
        "green_pin": "esp:D26",
        "btn_pin": "esp:D32",
        "servo_pin": "esp:D18"
    },
    {
        "id": 2,
        "name": "Cell 2",
        "red_pin": "esp:D14",
        "green_pin": "esp:RX2",
        "btn_pin": "esp:D33",
        "servo_pin": "esp:D19"
    },
    {
        "id": 3,
        "name": "Cell 3",
        "red_pin": "esp:D13",
        "green_pin": "esp:D12",
        "btn_pin": "esp:D27",
        "servo_pin": "esp:D5"
    },
    {
        "id": 4,
        "name": "Cell 4",
        "red_pin": "esp:D4",
        "green_pin": "esp:D2",
        "btn_pin": "esp:D23",
        "servo_pin": "esp:TX2"
    }
]

def parse_grid_str(grid_str):
    grid_str = grid_str.lower().strip()
    if 'x' in grid_str:
        parts = grid_str.split('x')
        return int(parts[0]), int(parts[1])
    try:
        count = int(grid_str)
        if count == 1:
            return 1, 1
        elif count == 2:
            return 1, 2
        elif count <= 4:
            return 2, 2
        elif count <= 16:
            return 4, 4
        elif count <= 64:
            return 8, 8
        else:
            return 2, 2
    except ValueError:
        return 2, 2

def generate_diagram(rows, cols):
    total_cells = rows * cols
    if total_cells > 64:
        total_cells = 64
        rows = 8
        cols = 8

    parts = [
        {
            "type": "wokwi-esp32-devkit-v1",
            "id": "esp",
            "top": 0,
            "left": 0,
            "attrs": {}
        },
        {
            "type": "board-ssd1306",
            "id": "oled",
            "top": 0,
            "left": 250,
            "attrs": { "i2cAddress": "0x3C" }
        },
        {
            "type": "wokwi-ds1307",
            "id": "rtc",
            "top": 140,
            "left": 250,
            "attrs": {}
        },
        {
            "type": "wokwi-buzzer",
            "id": "bz1",
            "top": 240,
            "left": 250,
            "attrs": { "volume": "1.0" }
        }
    ]

    connections = [
        [ "esp:TX0", "$serialMonitor:RX", "", [] ],
        [ "esp:RX0", "$serialMonitor:TX", "", [] ],
        [ "oled:VCC", "esp:3V3", "red", [] ],
        [ "oled:GND", "esp:GND.1", "black", [] ],
        [ "oled:SDA", "esp:D21", "green", [] ],
        [ "oled:SCL", "esp:D22", "blue", [] ],
        [ "rtc:5V", "esp:3V3", "red", [] ],
        [ "rtc:GND", "esp:GND.1", "black", [] ],
        [ "rtc:SDA", "esp:D21", "green", [] ],
        [ "rtc:SCL", "esp:D22", "blue", [] ],
        [ "bz1:1", "esp:GND.1", "black", [] ],
        [ "bz1:2", "esp:D15", "gold", [] ]
    ]

    # Grid layout coordinates
    # Each row is 115px high; each column is 300px wide
    row_height = 115
    col_width = 300
    base_top = -(rows * row_height) - 10

    for i in range(total_cells):
        cell_id = i + 1
        r = i // cols
        c = i % cols

        top = base_top + (r * row_height)
        left = 140 + (c * col_width)

        # 1. RGB LED
        parts.append({
            "type": "wokwi-rgb-led",
            "id": f"rgb{cell_id}",
            "top": top,
            "left": left,
            "attrs": { "common": "cathode" }
        })

        # 2. Pushbutton
        parts.append({
            "type": "wokwi-pushbutton",
            "id": f"btn{cell_id}",
            "top": top,
            "left": left + 80,
            "attrs": { "color": "green", "label": f"Btn {cell_id}" }
        })

        # 3. Servo Motor
        parts.append({
            "type": "wokwi-servo",
            "id": f"servo{cell_id}",
            "top": top,
            "left": left + 170,
            "attrs": {}
        })

        # Power connections for all cells
        connections.append([ f"rgb{cell_id}:COM", "esp:GND.1", "black", [] ])
        connections.append([ f"btn{cell_id}:2.l", "esp:GND.1", "black", [] ])
        connections.append([ f"servo{cell_id}:GND", "esp:GND.1", "black", [] ])
        connections.append([ f"servo{cell_id}:V+", "esp:3V3", "red", [] ])

        # Cells 1 to 4: Wire to physical ESP32 GPIOs
        if i < len(CELL_PINS):
            cfg = CELL_PINS[i]
            connections.append([ f"rgb{cell_id}:R", cfg["red_pin"], "red", [] ])
            connections.append([ f"rgb{cell_id}:G", cfg["green_pin"], "green", [] ])
            connections.append([ f"btn{cell_id}:1.l", cfg["btn_pin"], "purple" if cell_id % 2 != 0 else "orange", [] ])
            connections.append([ f"servo{cell_id}:PWM", cfg["servo_pin"], "cyan", [] ])

    diagram_data = {
        "version": 1,
        "author": "Nevin",
        "editor": "wokwi",
        "parts": parts,
        "connections": connections,
        "serialMonitor": {
            "display": "always"
        }
    }

    return diagram_data, total_cells

def main():
    parser = argparse.ArgumentParser(description="Configure Wokwi diagram.json for matrix grids.")
    parser.add_argument("grid", nargs="?", default=None, help="Grid size (e.g. 1x1, 1x2, 2x1, 2x2, 4x4, 8x8)")
    parser.add_argument("--config", default=CONFIG_FILE, help="Path to grid_config.json")
    args = parser.parse_args()

    grid_str = args.grid

    if not grid_str and os.path.exists(args.config):
        try:
            with open(args.config, "r") as f:
                data = json.load(f)
                grid_str = data.get("grid", "2x2")
                print(f"[CONFIG] Read grid '{grid_str}' from {args.config}")
        except Exception as e:
            print(f"[WARN] Failed to read {args.config}: {e}")
            grid_str = "2x2"
    elif not grid_str:
        grid_str = "2x2"

    rows, cols = parse_grid_str(grid_str)
    diagram, total_cells = generate_diagram(rows, cols)

    with open(DIAGRAM_FILE, "w") as f:
        json.dump(diagram, f, indent=2)

    # Save to grid_config.json as the persistent configuration variable
    config_data = {
        "grid": f"{rows}x{cols}",
        "rows": rows,
        "cols": cols,
        "total_compartments": total_cells,
        "physical_direct_gpio_cells": min(total_cells, 4),
        "virtual_expander_cells": max(0, total_cells - 4),
        "note": "ESP32 DevKit V1 has 19 usable GPIOs, allowing 4 direct physical GPIO compartments. Grids larger than 4 cells (4x4, 8x8) are rendered on canvas with power buses and managed by the firmware HAL."
    }
    with open(CONFIG_FILE, "w") as f:
        json.dump(config_data, f, indent=2)

    print(f"============================================================")
    print(f" SUCCESS: diagram.json updated for [{rows}x{cols}] Grid ({total_cells} Compartments)!")
    print(f" Servos generated on canvas : {total_cells}")
    print(f" Buttons generated on canvas: {total_cells}")
    print(f" LEDs generated on canvas   : {total_cells}")
    print(f" Configuration saved to     : {CONFIG_FILE}")
    print(f"------------------------------------------------------------")
    print(f" IMPORTANT: In Wokwi (VS Code or browser), you must:")
    print(f"   1. Click STOP (Red Square) in the simulation toolbar.")
    print(f"   2. Click START (Green Play button) to load the new canvas.")
    print(f"============================================================")

if __name__ == "__main__":
    main()
