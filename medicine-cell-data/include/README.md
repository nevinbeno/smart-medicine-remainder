# The Headers
## Structure
```txt
    .
    ├── cell.hpp
    ├── configuration.hpp
    ├── configuration_loader.hpp
    ├── medicine.hpp
    ├── README.md
    └── schedule.hpp

```
## Brief desc about the files: 
1. `schedule.hpp`
    - Defines what does one medicine schedule looks like in C++. 
2. `medicine.hpp`: 
    - Define what a Medicine object looks like in C++. 
3. `cell.hpp`: 
    - Defines the various states of a cell, and declares the operations on a cell. 
    - The implementations of the functions declared in this header file are contained in the `src/cell.cpp` file. 
4. `configuration.hpp`: 
    - This file defines the structure that holds the JSON information inside the ESP32. 
    - It defines the data structure that the JSON will be loaded into. 
5. `configuration_loader.hpp`: 
    - Defines the function which can parse / extract the JSON into the ESP32. It also refers the configuration.hpp file to specify how the extracted info is to be stored in runtime (to specify the RuntimeConfig). 
    - This file still does not explains "how", but just tells that the function exists. The "how" is answered using the implementation in `src/configuration_loader.cpp`

## Link between `configuration.hpp` and `configuration_loader.hpp`
```txt
    configuration.json
         │
      JSON data
        ↓
    configuration_loader.cpp
         │
     parses + extracts data
        ↓
    RuntimeConfiguration
        ↑
        │
    defined by
        │
    configuration.hpp
```
