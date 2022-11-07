import tkinter as tk
from abc import ABC, abstractmethod
from tkinter import ttk

import psycopg

from preprocessing import QueryPlanTree


class App(tk.Tk):
    def __init__(self, cursor: psycopg.Cursor):
        super().__init__()

        self.geometry("800x600")
        self.title("Query Analyzer")

        self.ctx = Context(cursor)

        self.topLabel = tk.Label(self, text="Enter SQL query below").grid(column=0, row=0)

        self.annotated_query_frame = AnnotatedQueryFrame(self, self.ctx)
        self.annotated_query_frame.grid(column=0, row=2)
        self.ctx.frames["annotated_query"] = self.annotated_query_frame

        self.visualize_query_plan_frame = VisualizeQueryPlanFrame(self, self.ctx)
        self.visualize_query_plan_frame.grid(column=1, row=1, rowspan=2)
        self.ctx.frames["visualize_query_plan"] = self.visualize_query_plan_frame

        self.input_query_frame = InputQueryFrame(self, self.ctx)
        self.input_query_frame.grid(column=0, row=1)
        self.ctx.frames["input_query"] = self.input_query_frame


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

        self.annotated_query_label = ttk.Label(self, text="Annotation")
        self.annotated_query_label.grid(column=0,row=0)

    def update_changes(self, *args, **kwargs):
        self.annotated_query_label["text"] = kwargs["qptree"].get_annotation()

class VisualizeQueryPlanFrame(ttk.Frame, Updatable):
    def __init__(self, master: tk.Misc, ctx: Context):
        super().__init__(master)
        self.ctx = ctx

        self.visualize_query_plan_label = ttk.Label(self, text="Visualization")
        self.visualize_query_plan_label.grid()
    
    def update_changes(self, *args, **kwargs):
        self.visualize_query_plan_label["text"] = kwargs["qptree"].get_visualization()

class InputQueryFrame(ttk.Frame, Updatable):
    def __init__(self, master: tk.Misc, ctx: Context):
        super().__init__(master)

        self.input_query_text = tk.Text(self, height=10, width=50)
        self.input_query_text.grid(row=0)

        self.analyze_query_button = ttk.Button(self, text="Analyze Query", command=self.analyze_query, padding=10)
        self.analyze_query_button.grid(row=1)

        self.ctx = ctx
    
    def analyze_query(self):
        input_query = self.input_query_text.get("1.0", "end-1c")
        self.ctx.vars["input_query"].set(input_query)

        qptree = QueryPlanTree.from_query(input_query, self.ctx.cursor)
        self.ctx.frames["visualize_query_plan"].update_changes(qptree=qptree)
        self.ctx.frames["annotated_query"].update_changes(qptree=qptree)

    def update_changes(self, *args, **kwargs):
        pass
