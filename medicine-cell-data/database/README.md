# Generating `medicine.db` structure from `scheme.sql`
1. Install `sqlite3`
```bash
sudo apt install sqlite3
```
2. Get into `database/`
```bash
cd database
```
3. Run: 
```bash
sqlite3 medicine.db < schema.sql
```