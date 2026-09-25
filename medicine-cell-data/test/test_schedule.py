from backend.schedule import (
    add_schedule,
    get_schedule,
    get_medicine_schedules,
    update_schedule,
    remove_schedule
)


# Add schedules
add_schedule("MED001", 8, 0, 1)
add_schedule("MED001", 20, 0, 1)

add_schedule("MED002", 22, 0, 1)
add_schedule("MED003", 13, 0, 1)
add_schedule("MED003", 22, 0, 1)
add_schedule("MED004", 7, 0, 1)


# print("MED001 schedules:")
# print(get_medicine_schedules("MED001"))


# print("\nFirst schedule:")
# schedule = get_schedule(1)
# print(schedule)


# print("\nUpdating schedule 1:")
# print(update_schedule(1, 8, 30, 2))
# print(get_schedule(1))


# print("\nRemoving schedule 2:")
# print(remove_schedule(2))


# print("\nMED001 schedules after changes:")
# print(get_medicine_schedules("MED001"))