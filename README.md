## Tutorials & References: 
- [Wokwi in VS Code (YouTube)](https://www.youtube.com/watch?v=ECNTyMm_5PE)
- [Wokwi Docs](https://docs.wokwi.com/)

## VS Code Extensions Required
- [extensions.md](docs/extensions.md)

## `diagram.json` and Wokwi license generation
- You will need to generate a wokwi license (free version, with a 30 days validity). This is required for the `diagram.json` file. 
- Once you get the License, come back to VS Code, press `F1`, type `wokwi`, and click the `Wokwi: Manually enter License key`. 
- I'd recommend to store the key in an `.env` file. 
- Drag and drop option won't be available in the VS Code extension for free users (available only for Hobby+ and Pro users). So: 
    - Create `diagram.json` file. 
    - Open in the VS Code text editor. 
    - Add the code. 

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

## Dose, Stock & Confirmation Management
- Design, behavior rules, and integration guidance: [docs/dose-stock-confirmation.md](docs/dose-stock-confirmation.md)
- Hardware-independent dose and inventory logic: [include/dose_manager.h](include/dose_manager.h)
- Automated native tests: run `pio test -e native`