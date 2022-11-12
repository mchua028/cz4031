from typing import Optional, Self

import psycopg

SCAN_TYPES = {"Bitmap Heap Scan", "Bitmap Index Scan", "Index Scan", "Index Only Scan", "Seq Scan", "Tid Scan"}
JOIN_TYPES = {"Hash Join", "Merge Join", "Nested Loop"}

class Relation:
	def __init__(self, relation_name: str, alias: str):
		self.relation_name = relation_name
		self.alias = alias

	"""
	When with_duplicate_alias = True, alias is included only if alias != relation name
	"""
	def __str__(self, with_duplicate_alias=False) -> str:
		if not with_duplicate_alias and self.relation_name == self.alias:
			return self.relation_name
		else:
			return f"{self.relation_name} {self.alias}"

class QueryPlanTreeNode:
	def __init__(self, info: dict={}, children: list[Self] = [], involving_relations: set[Relation] = set()):
		self.info = info
		self.children = children
		self.involving_relations = involving_relations

	def get_primary_info(self) -> dict:
		# List of node types -> https://github.com/postgres/postgres/blob/master/src/backend/commands/explain.c#L1191
		primary_info = {}
		for k, v in self.info.items():
			if k == "Node Type" or k == "Relation Name" \
				or k == "Alias" or k == "Group Key" or k == "Strategy" \
				or ("Filter" in k and "Removed by" not in k) or "Cond" in k:
				primary_info[k] = v

		return primary_info

	def get_cost(self) -> float:
		cost: float = self.info["Total Cost"]
		for child in self.children:
			cost -= child.info["Total Cost"]
		return round(cost, 1)

class QueryPlanTree:
	def __init__(self, query: str=""):
		self.root: Optional[QueryPlanTreeNode] = None
		self.scan_nodes: list[QueryPlanTreeNode] = []
		self.join_nodes: list[QueryPlanTreeNode] = []
		self.query = query

	@staticmethod
	def from_query(query: str, cursor: psycopg.Cursor):
		plan = query_plan(query, cursor)
		return QueryPlanTree.from_plan(plan, query)

	@staticmethod
	def from_plan(plan: dict, query: str):
		qptree = QueryPlanTree(query)
		qptree.root = qptree._build(plan)
		return qptree

	def _build(self, plan: dict):
		# Post-order traversal
		if "Node Type" not in plan:
			return None

		# Build subtrees
		children: list[QueryPlanTreeNode] = []
		involving_relations = set()

		subplans: Optional[list[dict]] = plan.get("Plans")
		if subplans is not None:
			for subplan in subplans:
				child = self._build(subplan)
				children.append(child)
				involving_relations.update(child.involving_relations)

		info = {k: v for k, v in plan.items() if k != "Plans"}
		if "Relation Name" in plan and "Alias" in plan:
			involving_relations.add(Relation(plan["Relation Name"], plan["Alias"]))

		node = QueryPlanTreeNode(info, children, involving_relations)
		if plan["Node Type"] in SCAN_TYPES:
			self.scan_nodes.append(node)
		elif plan["Node Type"] in JOIN_TYPES:
			self.join_nodes.append(node)

		return node

	def get_visualization(self):
		return QueryPlanTree._get_visualization_helper(self.root, 0)

	@staticmethod
	def _get_visualization_helper(node: Optional[QueryPlanTreeNode], level: int):
		if node is None:
			return ""

		children_visualizations = []
		for child in node.children:
			child_visualization = QueryPlanTree._get_visualization_helper(child, level + 1)
			if child_visualization == "":
				continue
			children_visualizations.append(f"\n{child_visualization}")

		return f"{':   ' * level}-> {node.get_primary_info()}" + "".join(children_visualizations)

def query_plan(query: str, cursor: psycopg.Cursor) -> dict:
	cursor.execute(f"EXPLAIN (FORMAT JSON) {query}")
	return cursor.fetchone()[0][0]["Plan"]
