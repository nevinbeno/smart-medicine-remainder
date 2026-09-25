from backend.medicine import (
    add_medicine,
    get_medicine,
    list_medicines,
    update_medicine,
    remove_medicine
)

# remove_medicine("MED001")

# add_medicine("MED001", "Paracetamol", 3)
# add_medicine("MED002", "Cetirizine", 2)
# add_medicine("MED003", "Amoxyllin", 3)
# add_medicine("MED004", "Pantocid", 3)

# # print("After adding:")
# print(list_medicines())

# # print("\nMED001:")
# # print(get_medicine("MED001"))

# update_medicine("MED001", "Paracetamol 500mg", 2)

# print("\nAfter update:")
# print(get_medicine("MED001"))

# add_medicine("MED002", "Pantodac", 2)

# print("\nAfter adding MED002:")
print(list_medicines())

# entries = list_medicines()
# for item in entries: 
#     med_id = item[0]
#     print(f"Removing {med_id}: ")
#     # print(type(med_id))
#     remove_medicine(med_id)
# print(list_medicines())
update_medicine("MED003", "Amoxicillin", 3)
print(list_medicines())