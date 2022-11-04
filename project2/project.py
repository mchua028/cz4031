import psycopg

from interface import App

if __name__ == "__main__":
    conninfo = "postgresql://postgres:1121@localhost:5432/tpchimported"

    with psycopg.connect(conninfo) as conn:
        with conn.cursor() as cursor:
            app = App(cursor)
            app.mainloop()
