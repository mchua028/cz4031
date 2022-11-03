import psycopg

from interface import App

if __name__ == "__main__":
    conninfo = "postgresql://postgres:Shyamvar1993@localhost:5432/tpchimported"

    with psycopg.connect(conninfo) as conn:
        with conn.cursor() as cursor:
            app = App(cursor)
            app.mainloop()
