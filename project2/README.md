# CZ4031 - Project 2

## Installation Instructions

There are several ways in which the project can be run, either via PyCharm IDE or via the command line.  
Requirements: 
- Python version >= 3.11.0 
- Python Virtual Environment
- PostgresSQL
- Database set up in PostgresSQL

**Note: It is possible to create virtual environment within PyCharm IDE. If command line is used, please follow the instructions below**

### Virtual Environment Setup

#### **Windows User**


1. Open up command line
2. Change directory to folder `project2/`

```bat
cd project2
```
3. Creating the virtual environment

```bat
python -m venv venv
```
4. Activating the virtual environment

```bat
venv\Scripts\activate.bat
```

#### **MAC/ Linux User**

1. Open up terminal
2. Change directory to folder `project2/`

```bat
cd project2
```
3. Creating the virtual environment

```bat
python -m venv venv
```
4. Activating the virtual environment
```bash
source venv/bin/activate
```

### Installing/ Updating Dependencies

Dependencies for this project can be installed using the following commands.  

- Install dependencies

```bash
pip install -r requirements.txt
```

- Update dependencies

```bash
pip freeze > requirements.txt
```

### Starting up the project

```bash
python ./project.py
```

### Establishing connection with POSTGRES database

Enter the following connection string into the `Postgres connection string` input

```bash
postgresql://<POSTGRES USERNAME>:<DATABASE PASSWORD>@localhost:5432/<DATABASE NAME>
```

### Entering query

The user's query can be entered into the textbox, and the results will be displayed upon clicking the `Analyze Query` button

#### Sample Test

The following sample test uses the `TPC-H` database as a source of data.

1. Enter Query
```postgres
SELECT customer.c_custkey 
FROM (SELECT * FROM customer WHERE customer.c_custkey > 1) AS customer, nation, orders 
WHERE customer.c_nationkey = nation.n_nationkey AND customer.c_custkey = orders.o_custkey
ORDER BY customer.c_nationkey, nation.n_nationkey;
```
2. Click `Analyze Query`
3. Annotation Results Display  

<img 
    style="display: block; 
           margin-left: auto;
           margin-right: auto;"
    src="./assets/sample_test_result.png" 
    alt="Our logo">
</img>

### Packages Used  

| Package Name | Version | 
| ----------- | ----------- |
| psycopg | 3.1.4 |
| psycopg-binary | 3.1.4 |
| typing_extensions | 4.4.0 |  

### Additional Information

| Resource      | Description |
| ----------- | ----------- |
| [Virtual Environment](https://realpython.com/python-virtual-environments-a-primer/)      | Virtual Environments In Python       |
| [PostgresSQL Optimizer](https://www.postgresql.org/docs/current/planner-optimizer.html)   | Planner/ Optimizer in PostgresSQL        |
| [Scan Methods](https://severalnines.com/blog/overview-various-scan-methods-postgresql/)  | Overview of Various Scan Methods in PostgresSQL
