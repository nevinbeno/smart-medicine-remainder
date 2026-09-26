# Generating `medicine.db` structure from `scheme.sql`
At first, we need to generate the database file using the schema we've already defined. Please follow the instructions: 
## Steps: 
1. Install `sqlite3`
```bash
sudo apt install sqlite3
```
2. Get into `database` folder: 
```bash
cd Smart-Medicine-Remainder/medicine-cell-data/database
```
3. Run: 
```bash
sqlite3 medicine.db < schema.sql
```
## Structure: 
```txt
    .
    ├── medicine.db
    ├── README.md
    └── schema.sql

    1 directory, 3 files
```
## Note: 
- This creates a file `medicine.db` inside the folder. 
- No data is now added into the database, just the file is created with according to the schema specified. 
- You may install VS Code extensions like SQLite3 viewer for viewing the database. 