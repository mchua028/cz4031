import tkinter as tk
from abc import ABC, abstractmethod
from tkinter import messagebox, ttk
from typing import Optional

import psycopg
from annotation import get_annotation, get_visualization

from preprocessing import QueryPlanTree
from PIL import Image

class App(tk.Tk):
    def __init__(self):
        super().__init__()

        self.geometry("1280x600")
        self.title("Query Analyzer")
        self.resizable(False, False)
        self.ctx = Context()

        self.container = ttk.Frame(self)
        self.container.pack(fill=tk.BOTH, expand=1)

        self.canvas = tk.Canvas(self.container)
        self.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=1)
        self.canvas.bind_all("<MouseWheel>", self._on_mousewheel)

        self.scroll_bar = ttk.Scrollbar(self.container, orient="vertical", command=self.canvas.yview)
        self.scroll_bar.pack(side=tk.RIGHT, fill=tk.Y)

        self.main_frame = ttk.Frame(self.canvas, width=1000, height=100)
        self.main_frame.bind(
            "<Configure>", lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all"))
        )
        self.canvas.create_window((0, 0), window=self.main_frame, anchor="nw")
        self.canvas.configure(yscrollcommand=self.scroll_bar.set)

        self.connection_frame = ConnectionFrame(self.main_frame, self.ctx)
        self.connection_frame.grid(column=0, row=0, columnspan=6)
        self.ctx.frames["connection"] = self.connection_frame

        self.input_query_frame = InputQueryFrame(self.main_frame, self.ctx)
        self.input_query_frame.grid(column=0, row=1, columnspan=6)
        self.ctx.frames["input_query"] = self.input_query_frame

        self.annotated_query_frame = AnnotatedQueryFrame(self.main_frame, self.ctx)
        self.annotated_query_frame.grid(column=0, row=2, columnspan=3,sticky='n')
        self.ctx.frames["annotated_query"] = self.annotated_query_frame

        self.visualize_query_plan_frame = VisualizeQueryPlanFrame(self.main_frame, self.ctx)
        self.visualize_query_plan_frame.grid(column=3, row=2, columnspan=3,sticky='n')
        self.ctx.frames["visualize_query_plan"] = self.visualize_query_plan_frame

    def _on_mousewheel(self, event):
        self.canvas.yview_scroll(int(-1*(event.delta/120)), "units")

class Updatable(ABC):
    @abstractmethod
    def update_changes(self, *args, **kwargs):
        pass

class Context:
    def __init__(self) -> None:
        self.vars: dict[str, tk.Variable] = {
            "input_query": tk.StringVar(),
        }
        self.frames: dict[str, Updatable] = {}
        self.cursor: Optional[psycopg.Cursor] = None

class AnnotatedQueryFrame(ttk.Frame, Updatable):
    def __init__(self, master: tk.Misc, ctx: Context):
        super().__init__(master)
        self.ctx = ctx

        self.top_label = tk.Label(self, text="Annotation", font=("Helvetica", 18, "bold"))
        self.top_label.grid(column=1, row=6, columnspan=5)
        self.annotated_query_label = ttk.Label(self, wraplength=600, font=("Helvetica", 10), width=90, anchor="center")
        self.annotated_query_label.grid(column=1, row=7, columnspan=5,sticky='n')

    def update_changes(self, *args, **kwargs):
        if kwargs.get("qptree") is None:
            self.annotated_query_label["text"] = ""
        else:
            self.annotated_query_label["text"] = get_annotation(kwargs["qptree"], self.ctx.cursor)

class VisualizeQueryPlanFrame(ttk.Frame, Updatable):
    def __init__(self, master: tk.Misc, ctx: Context):
        super().__init__(master)
        self.ctx = ctx

        self.top_label = tk.Label(self, text="Visualization", font=("Helvetica", 18, "bold"))
        self.top_label.grid(column=7, row=6, columnspan=5)
        self.visualize_query_plan_label = ttk.Label(self, wraplength=500, font=("Helvetica", 10), width=70, anchor="center")
        self.visualize_query_plan_label.grid(column=7, row=7, columnspan=5,sticky='n')
        self.view_qep_tree_btn = ttk.Button(self, text="View QEP tree", command=self.view_qep_tree, padding=10, state=["disabled"])
        self.view_qep_tree_btn.grid(column=7, row=7, columnspan=5)
        self.qep_tree = None

    def update_changes(self, *args, **kwargs):
        if kwargs.get("qptree") is None:
            self.visualize_query_plan_label["text"] = ""
            self.view_qep_tree_btn.state(["disabled"])
        else:
            self.qep_tree = kwargs["qptree"]
            self.view_qep_tree_btn.state(["!disabled"])
    
    def view_qep_tree(self):
        img_file_name = get_visualization(self.qep_tree)
        with Image.open(img_file_name) as im:
            im.show()
    

class InputQueryFrame(ttk.Frame, Updatable):
    def __init__(self, master: tk.Misc, ctx: Context):
        super().__init__(master)

        self.top_label = tk.Label(self, text="Enter SQL query below", font=("Helvetica", 18, "bold"))
        self.top_label.grid(row=1)
        self.input_query_text = tk.Text(self, height=10, width=50, state="disabled")
        self.input_query_text.grid(row=2)

        self.analyze_query_button = ttk.Button(self, text="Analyze Query", command=self.analyze_query, padding=10, state=["disabled"])
        self.analyze_query_button.grid(row=3)

        self.ctx = ctx

    def analyze_query(self):
        input_query = self.input_query_text.get("1.0", "end-1c")
        self.ctx.vars["input_query"].set(input_query)

        qptree: Optional[QueryPlanTree] = None
        try:
            qptree = QueryPlanTree.from_query(input_query, self.ctx.cursor,True)
        except Exception as err:
            messagebox.showerror("Error", err)

        self.ctx.cursor.connection.rollback()
        self.ctx.frames["visualize_query_plan"].update_changes(qptree=qptree)
        self.ctx.frames["annotated_query"].update_changes(qptree=qptree,query=input_query)

    def update_changes(self, *args, **kwargs):
        input_query = self.input_query_text.get("1.0", "end-1c")
        # Reset input
        self.input_query_text.delete("1.0", tk.END)
        self.ctx.frames["visualize_query_plan"].update_changes()
        self.ctx.frames["annotated_query"].update_changes(query=input_query)

        if kwargs.get("disabled"):
            self.input_query_text.config(state="disabled")
            self.analyze_query_button.state(["disabled"])
        else:
            self.input_query_text.config(state="normal")
            self.analyze_query_button.state(["!disabled"])


class ConnectionFrame(ttk.Frame, Updatable):
    def __init__(self, master: tk.Misc, ctx: Context):
        super().__init__(master)

        self.top_label = ttk.Label(self, text="Postgres connection string")
        self.top_label.grid(column=0, row=0)

        self.connection_text = ttk.Entry(self, width=60)
        self.connection_text.grid(column=1, row=0)

        self.connect_button = ttk.Button(self, text="Connect", command=self.connect)
        self.connect_button.grid(column=2, row=0)

        self.connection_status = tk.Label(self, text="Not Connected", foreground="red")
        self.connection_status.grid(column=3, row=0)

        self.ctx = ctx

    def connect(self):
        try:
            conninfo = self.connection_text.get()
            self.ctx.cursor = psycopg.connect(conninfo).cursor()
            self.ctx.frames["input_query"].update_changes(disabled=False)
            self.connection_status["text"] = "Connected"
            self.connection_status["foreground"] = "green"
        except Exception as err:
            self.ctx.cursor = None
            self.ctx.frames["input_query"].update_changes(disabled=True)
            self.connection_status["text"] = "Not Connected"
            self.connection_status["foreground"] = "red"
            messagebox.showerror("Error", err)

    def update_changes(self, *args, **kwargs):
        pass
    
