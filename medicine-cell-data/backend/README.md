# Backend Package
## Purpose: 
- This package defines the methods for all the database communication in this MEDICINE-CELL-DATA Module. 
- These files contains function definitions, which are invoked by the test files inside `test/` folder. The test files uses functions in the current folder (`backend`) to communicate with the database. 

## Structure: 
```txt
.
├── cell.py
├── database.py
├── __init__.py
├── json_generator.py
├── medicine.py
├── README.md
├── relationship.py
└── schedule.py
```
## A small description about the files: 
1. `__init__.py`
    - To consider this folder as a python package. 
2. `cell.py` | `medicine.py` | `schedule.py`
    - Contains the function definitions to interact with tables like cell, medicine, schedule respectively. 
3. `relationship.py`
    - A file containing the methods to get the complete details of a cell and its associated medicine and their schedule. 
4. `json_generator.py`
    - The master file, which is responsible for generating the json file containing all the details of the current startup configuration of the system. 
    - The JSON is a snapshot of the configuration that the system needs.
    - The data in this json will be embedded into the ESP32 Microcontroller. 