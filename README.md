## Tutorials & References: 
- [Wokwi in VS Code (YouTube)](https://www.youtube.com/watch?v=ECNTyMm_5PE)
- [Wokwi Docs](https://docs.wokwi.com/)

## VS Code Extensions Required
- [extensions.md](docs/extensions.md)

## `diagram.json` and Wokwi license generation
- You will need to generate a wokwi license (free version, with a 30 days validity). This is required for the `diagram.json` file. 
- Once you get the License, come back to VS Code, press `F1`, type `wokwi`, and click the `Wokwi: Manually enter License key`. 
- I'd recommend to store the key in an `.env` file. 
- Drag and drop option won't be available in the VS Code extension for free users (available only for Hobby+ and Pro users).
- **To configure the physical grid on the canvas (e.g. 1x1, 1x2, 2x2, 4x4, 8x8):**
  - **Single Point of Entry (Recommended):** Set `"grid"` in `medicine-cell-data/config/configuration.json` (e.g. `"4x4"`, `"2x2"`, or `"auto"`). PlatformIO automatically synchronizes `diagram.json` and `grid_config.json` every time you build or click Play in Wokwi! Keep `cellCount == rows*cols == len(cells[])` (4/2x2, 16/4x4).
  - **Manual CLI (Alternative):** Run `python3 scripts/set_diagram_grid.py 4x4` (or `2x2`, `8x8`). This automatically generates and wires `diagram.json` with zero manual copy-pasting.
- **Cells 5+ are virtual expander** (ESP32 has no free GPIOs after 1-4 + I2C + buzzer = 19 pins). `Btn 5+` shows `(V)` orange and is power-only in Wokwi — confirm with serial `PRESS 5` / `PRESS 2,1`. LED/servo state is logged (`[VIRTUAL] ...`) + shown in `STATUS` with `V` flag. See `docs/cell_hardware_control.md`.

## System Arch (at the moment)
```txt
    .
    ├── diagram.json
    ├── docs
    │   └── extensions.md
    ├── .env
    ├── .git/
    ├── .gitignore
    ├── include
    │   └── README
    ├── lib
    │   └── README
    ├── .pio/
    ├── platformio.ini
    ├── README.md
    ├── sample
    │   ├── sample_diagram.json
    │   └── sample_main.cpp
    ├── src
    │   └── main.cpp
    ├── test
    │   ├── README
    │   └── Smart Medicine Remainder.code-workspace
    ├── .vscode/
    └── wokwi.toml

```

- Once you setup the PlatformIO IDE and other extensions, and create a project, you will get all the pre-built libraries. 
- You will need to create `.toml` file, `diagram.json` file. 
- You will need to edit the `src/main.cpp` file, `platformio.ini` file. 
- Folders like `docs/`, `sample/` are created by me, they are not pre-built. 