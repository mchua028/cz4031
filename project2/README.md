# CZ4031 - Project 2

## Installation Instructions

There are several ways in which the project can be run, either via PyCharm IDE or via the command line.  
Requirements: 
- Python version >= 3.11.0 
- PostgresSQL
- Database set up in PostgresSQL

### Steps to follow  

1. Change Directory to folder
```bash
cd project2
```

2. Installing/ Updating Dependencies  

Dependencies for this project can be installed using the following commands.  

- Install dependencies

```bash
pip install -r requirements.txt
```

- Update dependencies

```bash
pip freeze > requirements.txt
```

3. Running the project

```bash
python ./project.py
```

4. Establishing connection with POSTGRES database

Enter the following connection string into the `Postgres connection string` input

```bash
postgresql://<POSTGRES USERNAME>:<DATABASE PASSWORD>@localhost:5432/<DATABASE NAME>
```

5. Entering query

The user's query can be entered into the textbox, and the results will be displayed upon clicking the `Analyze Query` button

### Sample Test

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


### Additional Information

- [Virtual Environment](https://realpython.com/python-virtual-environments-a-primer/)   
- [PostgresSQL Optimizer](https://www.postgresql.org/docs/current/planner-optimizer.html)
- [Scan Methods](https://severalnines.com/blog/overview-various-scan-methods-postgresql/)  
