import os

Import("env")

env.Append(
    CPPPATH=[
        os.path.join(
            "$PROJECT_LIBDEPS_DIR",
            "$PIOENV",
            "ArduinoJson"
        ),
        os.path.join(
            "$PROJECT_LIBDEPS_DIR",
            "$PIOENV",
            "ArduinoJson",
            "src"
        )
    ]
)

env.BuildSources(
    os.path.join("$BUILD_DIR", "medicine-cell-data"),
    os.path.join("$PROJECT_DIR", "medicine-cell-data", "src")
)