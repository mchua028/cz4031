import tkinter as tk
from abc import ABC, abstractmethod
from tkinter import messagebox, ttk
from typing import Optional

import psycopg

from preprocessing import QueryPlanTree


class App(tk.Tk):
    def __init__(self, cursor: psycopg.Cursor):
        super().__init__()

        self.geometry("800x600")
        self.title("Query Analyzer")

        self.ctx = Context(cursor)

        self.connection_frame = ConnectionFrame(self, self.ctx)
        self.connection_frame.grid(column=0, row=0)
        self.ctx.frames["connection_frame"] = self.connection_frame

        self.input_query_frame = InputQueryFrame(self, self.ctx)
        self.input_query_frame.grid(column=0, row=1)
        self.ctx.frames["input_query"] = self.input_query_frame

        self.annotated_query_frame = AnnotatedQueryFrame(self, self.ctx)
        self.annotated_query_frame.grid(column=1, row=1)
        self.ctx.frames["annotated_query"] = self.annotated_query_frame

        self.visualize_query_plan_frame = VisualizeQueryPlanFrame(self, self.ctx)
        self.visualize_query_plan_frame.grid(column=2, row=1)
        self.ctx.frames["visualize_query_plan"] = self.visualize_query_plan_frame

class Updatable(ABC):
    @abstractmethod
    def update_changes(self, *args, **kwargs):
        pass

class Context:
    vars: dict[str, tk.Variable]
    frames: dict[str, Updatable]

    def __init__(self, cursor: psycopg.Cursor) -> None:
        self.vars = {
            "input_query": tk.StringVar(),
        }
        self.frames = {}
        self.cursor = cursor

class AnnotatedQueryFrame(ttk.Frame, Updatable):
    def __init__(self, master: tk.Misc, ctx: Context):
        super().__init__(master)
        self.ctx = ctx

        self.top_label = tk.Label(self, text="Annotation")
        self.top_label.grid(row=0)
        self.annotated_query_label = ttk.Label(self)
        self.annotated_query_label.grid(column=0, row=1)

    def update_changes(self, *args, **kwargs):
        if kwargs["qptree"] is None:
            self.annotated_query_label["text"] = ""
        else:
            self.annotated_query_label["text"] = kwargs["qptree"].get_annotation()

class VisualizeQueryPlanFrame(ttk.Frame, Updatable):
    def __init__(self, master: tk.Misc, ctx: Context):
        super().__init__(master)
        self.ctx = ctx

        self.top_label = tk.Label(self, text="Visualization")
        self.top_label.grid(row=0)
        self.visualize_query_plan_label = ttk.Label(self)
        self.visualize_query_plan_label.grid(row=1)

    def update_changes(self, *args, **kwargs):
        if kwargs["qptree"] is None:
            self.visualize_query_plan_label["text"] = ""
        else:
            self.visualize_query_plan_label["text"] = kwargs["qptree"].get_visualization()

class InputQueryFrame(ttk.Frame, Updatable):
    def __init__(self, master: tk.Misc, ctx: Context):
        super().__init__(master)

        self.top_label = tk.Label(self, text="Enter SQL query below")
        self.top_label.grid(row=0)
        self.input_query_text = tk.Text(self, height=10, width=50)
        self.input_query_text.grid(row=1)

        self.analyze_query_button = ttk.Button(self, text="Analyze Query", command=self.analyze_query, padding=10)
        self.analyze_query_button.grid(row=2)

        self.ctx = ctx

    def analyze_query(self):
        input_query = self.input_query_text.get("1.0", "end-1c")
        self.ctx.vars["input_query"].set(input_query)

        qptree: Optional[QueryPlanTree] = None
        try:
            qptree = QueryPlanTree.from_query(input_query, self.ctx.cursor)
        except Exception as err:
            messagebox.showerror("Error", err)

        self.ctx.cursor.connection.rollback()
        self.ctx.frames["visualize_query_plan"].update_changes(qptree=qptree)
        self.ctx.frames["annotated_query"].update_changes(qptree=qptree)

    def update_changes(self, *args, **kwargs):
        pass

class ConnectionFrame(ttk.Frame, Updatable):
    def __init__(self, master: tk.Misc, ctx: Context):
        super().__init__(master)

        self.top_label = ttk.Label(self, text="Postgres connection string")
        self.top_label.grid(column=0, row=0)

        self.connection_text = ttk.Entry(self, width=60)
        self.connection_text.grid(column=1, row=0)

        self.connect_button = ttk.Button(self, text="Connect", command=self.connect)
        self.connect_button.grid(column=2, row=0)

        self.ctx = ctx

    def connect(self):
        pass

    def update_changes(self, *args, **kwargs):
        pass
