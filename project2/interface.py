import tkinter as tk
import typing
from abc import ABC, abstractmethod
from tkinter import ttk

import psycopg
import sqlparse
from sqlparse.sql import IdentifierList, Identifier
from sqlparse.tokens import Keyword, DML

from preprocessing import QueryPlanTree
import preprocessing


class App(tk.Tk):
    def __init__(self, cursor: psycopg.Cursor):
        super().__init__()

        self.geometry("1000x600")
        self.title("Query Analyzer")

        self.ctx = Context(cursor)

        self.topLabel = tk.Label(self, text="Enter SQL query below").grid(column=0, row=0)

        self.annotated_query_frame = AnnotatedQueryFrame(self, self.ctx)
        self.annotated_query_frame.grid(column=0, row=2)
        self.ctx.frames["annotated_query"] = self.annotated_query_frame

        self.visualize_query_plan_frame = VisualizeQueryPlanFrame(self, self.ctx)
        self.visualize_query_plan_frame.grid(column=1, row=2)
        self.ctx.frames["visualize_query_plan"] = self.visualize_query_plan_frame

        self.input_query_frame = InputQueryFrame(self, self.ctx)
        self.input_query_frame.grid(column=0, row=1)
        self.ctx.frames["input_query"] = self.input_query_frame

        self.annotation_table = annotationTableFrame(self, self.ctx)
        self.annotation_table.grid(column=0, row=4)
        self.ctx.frames["annotation_table"] = self.annotation_table


class Updatable(ABC):
    @abstractmethod
    def update_changes(self, *args, **kwargs):
        pass

class Context:
    vars: typing.Dict[str, tk.Variable]
    frames: typing.Dict[str, Updatable]

    def __init__(self, cursor: psycopg.Cursor) -> None:
        self.vars = {
            "input_query": tk.StringVar()
        }
        self.frames = {}
        self.cursor = cursor

class annotationTableFrame(ttk.Frame, Updatable):
    def __init__(self, master: tk.Misc, ctx: Context):
        super().__init__(master)
        self.ctx = ctx

        columns = ('1', '2', '3', '4')
        self.annotation_table = ttk.Treeview(self, columns=columns, show='headings')

        self.annotation_table.heading('1', text='Keyword')
        self.annotation_table.heading('2', text='Identifier')
        self.annotation_table.heading('3', text='Operation Chosen')
        self.annotation_table.heading('4', text='Reason')

        self.annotation_table.column('1', width=100, anchor=tk.W)
        self.annotation_table.column('2', width=300, anchor=tk.W)
        self.annotation_table.column('3', width=200, anchor=tk.W)
        self.annotation_table.column('4', width=500, anchor=tk.W)

        contacts = []
        contacts.append((f'FROM', f'customer', f'seq scan', f'resoning................'))
        contacts.append((f'', f'nation', f'seq scan', f'resoning................'))
        contacts.append((f'WHERE', f'customer.c_nationkey = nation.n_nationkey', f'Hash join', f'resoning................'))
        contacts.append((f'', f'customer.c_acctbal >= 1000', f'filtered during seq scan', f'resoning................'))
        contacts.append((f'', f'nation.n_nationkey >= 10', f'filtered during seq scan', f'resoning................'))
        contacts.append((f'', f'order by', f'Sort', f'resoning................'))

        for contact in contacts:
            self.annotation_table.insert('', tk.END, values=contact)

        self.annotation_table.grid(column=0,row=0, sticky='nsew')

    def update_changes(self, *args, **kwargs):
        pass

class AnnotatedQueryFrame(ttk.Frame, Updatable):
    def __init__(self, master: tk.Misc, ctx: Context):
        super().__init__(master)
        self.ctx = ctx

        self.annotated_query = ttk.Label(self, text="Annotation")
        self.annotated_query.grid(column=0,row=0)

        self.query = ttk.Label(self, text="")
        self.query.grid(column=0,row=2)

    def parseSql(self, query):
        #query = "SELECT * FROM (SELECT * FROM CUSTOMER WHERE ID > 10) WHERE ID <50 ;"
        parsed = sqlparse.parse(query)
        stmt = parsed[0]
        print(stmt.tokens)
         
        #return sqlparse.parse(query)

    def update_changes(self, *args, **kwargs):
        formatted = sqlparse.format(self.ctx.vars["input_query"].get(), reindent=True, keyword_case='upper')
        self.query["text"] = formatted
        self.parseSql(self.ctx.vars["input_query"].get())


class VisualizeQueryPlanFrame(ttk.Frame, Updatable):
    def __init__(self, master: tk.Misc, ctx: Context):
        super().__init__(master)
        self.ctx = ctx

        self.visualize_query_plan = ttk.Label(self, text="Visualization")
        self.visualize_query_plan.grid(column=0, row=0)
    
    def update_changes(self, *args, **kwargs):
        input_query = self.ctx.vars["input_query"].get()
        self.visualize_query_plan["text"] = str(QueryPlanTree(self.ctx.cursor, input_query))
        print(str(QueryPlanTree(self.ctx.cursor, input_query)))


class InputQueryFrame(ttk.Frame, Updatable):
    def __init__(self, master: tk.Misc, ctx: Context):
        super().__init__(master)

        self.input_query = tk.Text(self, height=10, width=50)
        self.input_query.grid(row=0)

        self.analyze_query_button = ttk.Button(self, text="Analyze Query", command=self.analyze_query, padding=10)
        self.analyze_query_button.grid(row=1)

        self.ctx = ctx
    
    def analyze_query(self):
        self.ctx.vars["input_query"].set(
            self.input_query.get("1.0", "end-1c")
        )
        self.ctx.frames["annotated_query"].update_changes()
        self.ctx.frames["visualize_query_plan"].update_changes()


        

    def update_changes(self, *args, **kwargs):
        pass
