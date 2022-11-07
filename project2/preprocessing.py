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
		self.join_nodes = {}

	# get the query plan from the query
	@staticmethod
	def from_query(query: str, cursor: psycopg.Cursor):
		plan = query_plan(query, cursor)
		# returns a QueryPlanTree
		return QueryPlanTree.from_plan(plan)

	# Builds the QueryPlanTree
	@staticmethod
	def from_plan(plan: dict):
		qptree = QueryPlanTree()
		qptree.root = qptree._build(plan)
		return qptree
	
	def _build(self, plan: dict):	
		# Post-order traversal
		if "Node Type" not in plan:
			return None

		# Build subtrees
		left: Optional[QueryPlanTreeNode] = None
		right: Optional[QueryPlanTreeNode] = None
		involving_relations = set()

		subplans: Optional[list[dict]] = plan.get("Plans")
		if subplans is not None:
			if len(subplans) >= 1:
				left = self._build(subplans[0])
				involving_relations.update(left.involving_relations)
			if len(subplans) >= 2:
				right = self._build(subplans[1])
				involving_relations.update(right.involving_relations)
		
		info = {k: v for k, v in plan.items() if k != "Plans"}
		if "Relation Name" in plan and "Alias" in plan:
			involving_relations.add(Relation(plan["Relation Name"], plan["Alias"]))

		return QueryPlanTreeNode(info, left, right, involving_relations)
	
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

# Generate the alternative Query plan
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

#  Generate the alternative q plan tree
#  returns a generator object
def alternative_query_plan_trees(query: str, cursor: psycopg.Cursor):
	print(query)
	for plan in alternative_query_plans(query, cursor):
		yield QueryPlanTree.from_plan(plan)

def traverse_aqp(node: Optional[QueryPlanTreeNode], join_nodes_dict: dict):
	if node is None:
		return ""
	traverse_aqp(node.left, join_nodes_dict)
	traverse_aqp(node.right, join_nodes_dict)
	
	node_info = node.info
	for inv_relation in node.involving_relations:
		print(inv_relation.relation_name)
	print("\n")

def handle_join_nodes(aqp_list: list[QueryPlanTree]) -> dict[str: dict]:
	join_nodes_dict = dict()
	for aqp in aqp_list: 
		print("AQP")
		print(aqp)
		print("\nTraversing the aqp\n")
		traverse_aqp(aqp.root, join_nodes_dict)