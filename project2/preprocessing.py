from typing import Optional, Self
import psycopg

from itertools import combinations, product

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

# Assume always binary tree
class QueryPlanTreeNode:
	def __init__(self, info: dict={}, left: Optional[Self]=None, right: Optional[Self]=None, involving_relations: set[Relation] = set()):
		self.info = info
		self.left = left
		self.right = right
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
				
class QueryPlanTree:
	root: Optional[QueryPlanTreeNode]
	scan_nodes: dict[str, QueryPlanTreeNode]

	def __init__(self):
		self.root = None
		self.scan_nodes = {}

	@staticmethod
	def from_query(query: str, cursor: psycopg.Cursor):
		plan = query_plan(query, cursor)
		return QueryPlanTree.from_plan(plan)

	@staticmethod
	def from_plan(plan: dict):
		qptree = QueryPlanTree()
		qptree.root = qptree._build(plan)
		return qptree
	
	def _build(self, plan: dict):	
		# Pre-order traversal
		if "Node Type" not in plan:
			return None
		
		info = {k: v for k, v in plan.items() if k != "Plans"}
		cur = QueryPlanTreeNode(info)

		# Track scan nodes
		if "Scan" in plan["Node Type"]:
			cur.involving_relations.add(
				Relation(plan["Relation Name"], plan["Alias"])
			)
			k = f"{plan['Relation Name']} {plan['Alias']}"
			self.scan_nodes[k] = cur

		# Build subtrees
		subplans: Optional[list[dict]] = plan.get("Plans")
		if subplans is not None:
			if len(subplans) >= 1:
				cur.left = self._build(subplans[0])
				cur.involving_relations.update(
					cur.left.involving_relations
				)
			if len(subplans) >= 2:
				cur.right = self._build(subplans[1])
				cur.involving_relations.update(
					cur.right.involving_relations
				)

		return cur
	
	def __str__(self):
		return QueryPlanTree._str_helper(self.root, 0) 

	@staticmethod
	def _str_helper(node: Optional[QueryPlanTreeNode], level: int):
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

def query_plan(query: str, cursor: psycopg.Cursor) -> dict:
	cursor.execute(f"EXPLAIN (FORMAT JSON) {query}")
	return cursor.fetchone()[0][0]["Plan"]

def alternative_query_plans(query: str, cursor: psycopg.Cursor):
	scan_types = ["bitmapscan", "indexscan", "indexonlyscan", "seqscan", "tidscan"]
	join_types = ["hashjoin", "mergejoin", "nestloop"]

	for disabled_scan_types, disabled_join_types in product(
		combinations(scan_types, len(scan_types) - 1),
		combinations(join_types, len(join_types) - 1),
	):
		# Enforce single scan type and single join type
		for disabled_type in disabled_scan_types + disabled_join_types:
			cursor.execute(f"SET LOCAL enable_{disabled_type}=false")

		yield query_plan(query, cursor)
		cursor.connection.rollback()

def alternative_query_plan_trees(query: str, cursor: psycopg.Cursor):
	for plan in alternative_query_plans(query, cursor):
		yield QueryPlanTree.from_plan(plan)
