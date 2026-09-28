import os

Import("env")

project_dir = env.subst("$PROJECT_DIR")
build_dir = env.subst("$BUILD_DIR")
libdeps_dir = env.subst("$PROJECT_LIBDEPS_DIR")
pio_env = env.subst("$PIOENV")

# Add Arduino framework library paths for Wire and SPI
try:
    framework_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
    if framework_dir:
        env.Append(
            CPPPATH=[
                os.path.join(framework_dir, "libraries", "Wire", "src"),
                os.path.join(framework_dir, "libraries", "SPI", "src"),
            ]
        )
except Exception:
    pass

# Append libdeps include paths so external module sources can find RTClib, BusIO, ArduinoJson
env.Append(
    CPPPATH=[
        os.path.join(libdeps_dir, pio_env, "RTClib"),
        os.path.join(libdeps_dir, pio_env, "RTClib", "src"),
        os.path.join(libdeps_dir, pio_env, "Adafruit BusIO"),
        os.path.join(libdeps_dir, pio_env, "ArduinoJson"),
        os.path.join(libdeps_dir, pio_env, "ArduinoJson", "src"),
    ]
)

# Include paths for scheduling-event-engine
sched_include = os.path.join(project_dir, "scheduling-event-engine", "include")
if os.path.exists(sched_include):
    env.Append(CPPPATH=[sched_include])

# Build scheduling-event-engine sources
sched_src = os.path.join(project_dir, "scheduling-event-engine", "src")
if os.path.exists(sched_src):
    env.BuildSources(
        os.path.join(build_dir, "scheduling-event-engine"),
        sched_src
    )

# Include paths for medicine-cell-data if present
med_include = os.path.join(project_dir, "medicine-cell-data", "include")
if os.path.exists(med_include):
    env.Append(CPPPATH=[med_include])

# Build medicine-cell-data sources if present
med_src = os.path.join(project_dir, "medicine-cell-data", "src")
if os.path.exists(med_src):
    env.BuildSources(
        os.path.join(build_dir, "medicine-cell-data"),
        med_src
    )
