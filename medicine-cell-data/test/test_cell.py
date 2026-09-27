from backend.cell import (
    add_cell,
    get_cell,
    list_cells,
    assign_medicine,
    remove_medicine,
    set_stock,
    disable_cell,
    enable_cell
)


# Create cells
add_cell("CELL01")
add_cell("CELL02")
add_cell("CELL03")
add_cell("CELL04")


print("Initial cells:")
print(list_cells())


# Assign medicines
assign_medicine("CELL01", "MED001")
assign_medicine("CELL02", "MED002")
assign_medicine("CELL03", "MED003")
assign_medicine("CELL04", "MED004")


print("\nAfter assigning medicines:")
print(list_cells())


# Set stock
set_stock("CELL01", 20)
set_stock("CELL02", 10)
set_stock("CELL03", 15)
set_stock("CELL04", 8)


print("\nAfter setting stock:")
print(list_cells())


# Disable CELL04
disable_cell("CELL04")

print("\nAfter disabling CELL04:")
print(list_cells())


# Try enabling CELL04
enable_cell("CELL04")

print("\nAfter enabling CELL04:")
print(list_cells())