from backend.relationship import (
    get_cell_medicine,
    get_cell_schedules,
    get_cell_details
)


print("=== CELL01 MEDICINE ===")
print(get_cell_medicine("CELL01"))


print("\n=== CELL03 MEDICINE ===")
print(get_cell_medicine("CELL03"))


print("\n=== CELL03 SCHEDULES ===")
print(get_cell_schedules("CELL03"))


print("\n=== CELL03 COMPLETE DETAILS ===")
print(get_cell_details("CELL03"))