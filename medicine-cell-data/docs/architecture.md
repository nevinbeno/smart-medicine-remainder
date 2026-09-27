# Medicine and Cell Data Architecture
## 1. Medicine Architecture
> This explains the stored data about medicine. 
- Each medicine should have its own: 
    ```txt
    Medicine ID
    Medicine name
    Low-Stock Threshold
    Schedule[]
    ```
- Each `schedule[]` entry contains: 
    ```txt
    Scheduled Time
    Dose Quantity
    ```

    ### Description: 
    - Schedule: The schedule represents the prescribed times at which the medicine should be taken and the quantity to be taken at each scheduled time.

## 2. Cell Architecture
> This defines what a cell logically represents. 
- A cell represents a physical storage compartment and its current logical state. 
- Each cell should contain details like: 
    ```txt
    Cell ID
    Assigned Medicine ID
    Current Stock
    Cell state
    ```

    ### Description: 
    - Assigned Medicine ID: establishes the relationship between
    a cell and the medicine stored in that cell.
    - Current Stock: represents the quantity of medicine currently
    available in that particular cell.
    - Low-Stock Threshold: defines when the stock in a cell becomes low.