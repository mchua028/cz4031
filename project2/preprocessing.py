from typing import List, Optional, Self
import json
import psycopg

# Assume always binary tree
class QueryPlanTreeNode:
	def __init__(self, info: dict, left: Optional[Self] = None, right: Optional[Self] = None):
		self.info = info
		self.left = left
		self.right = right
	
	def get_primary_info(self) -> dict:
		# List of node types -> https://github.com/postgres/postgres/blob/master/src/backend/commands/explain.c#L1191
		primary_info = {}
		for k, v in self.info.items():
			if k == "Node Type" or k == "Relation Name" \
				or k == "Alias" or k == "Group Key" or k == "Strategy" \
				or ("Filter" in k and "Removed by" not in k) or "Cond" in k:
				primary_info[k] = v
		
		return primary_info
				
class QueryPlanTree:
	root: Optional[QueryPlanTreeNode]

	def __init__(self):
		self.root = None
	
	def __init__(self, cursor: psycopg.Cursor, query: str):
		plan: dict = explain_query(cursor, query)["Plan"]
		self.root = self._build(plan)

	@staticmethod
	def _build(plan: dict) -> Optional[QueryPlanTreeNode]:
		if "Node Type" not in plan:
			return None
		
		info = {k: v for k, v in plan.items() if k != "Plans"}
		cur = QueryPlanTreeNode(info)

		subplans: Optional[List[dict]] = plan.get("Plans")
		if subplans is not None:
			if len(subplans) >= 1:
				cur.left = QueryPlanTree._build(subplans[0])
			if len(subplans) >= 2:
				cur.right = QueryPlanTree._build(subplans[1])

		return cur
	
	def __str__(self) -> str:
		return QueryPlanTree._str_helper(self.root, 0) 

	@staticmethod
	def _str_helper(node: Optional[QueryPlanTreeNode], level: int) -> str:
		if node is None:
			return ""
		
		left = QueryPlanTree._str_helper(node.left, level + 1)
		if left != "":
			left = "\n" + left

		right = QueryPlanTree._str_helper(node.right, level + 1)
		if right != "":
			right = "\n" + right 

		node_type: str = node.info["Node Type"]
		del node.info["Node Type"]

		return f"{'    ' * level}-> {node_type} {node.get_primary_info()}{left}{right}"

def explain_query(cursor: psycopg.Cursor, query: str, debug: bool = False) -> dict:
	cursor.execute(
		f"EXPLAIN (FORMAT JSON, ANALYZE) {query}"
	)
	query_explanation = cursor.fetchone()[0][0]
	if debug:
		print("Query Explanation:", json.dumps(query_explanation, indent=2))

	return query_explanation