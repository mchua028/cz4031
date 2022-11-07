from typing import Optional, Self
import psycopg

from itertools import combinations, product

SCAN_TYPES = {"Bitmap Heap Scan", "Bitmap Index Scan", "Index Scan", "Index Only Scan", "Seq Scan", "Tid Scan"}
SCAN_TYPE_FLAGS = {"bitmapscan", "indexscan", "indexonlyscan", "seqscan", "tidscan"}

JOIN_TYPES = {"Hash Join", "Merge Join", "Nested Loop"}
JOIN_TYPE_FLAGS ={"hashjoin", "mergejoin", "nestloop"}

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

	def get_all_nodes_info(self):
		nodes_info=[]
		if self is None:
			return nodes_info
		nodes_info.append(self.info)
		if self.right is not None:
			nodes_info=nodes_info+self.right.get_all_nodes_info()
		if self.left is not None:
			nodes_info=nodes_info+self.left.get_all_nodes_info()
		return nodes_info

class QueryPlanTree:
	root: Optional[QueryPlanTreeNode]
	scan_nodes: list[QueryPlanTreeNode]
	join_nodes: list[QueryPlanTreeNode]

	def __init__(self):
		self.root = None
		self.scan_nodes = []
		self.join_nodes = []

	@staticmethod
	def from_query(query: str, cursor: psycopg.Cursor):
		plan = query_plan(query, cursor)
		return QueryPlanTree.from_plan(plan)

	@staticmethod
	def from_plan(plan: dict):
		qptree = QueryPlanTree()
		qptree.root = qptree._build(plan)
		return qptree

	def get_annotation(self):
		return QueryPlanTree._get_annotation_helper(self.root, 1)[0]

	@staticmethod
	def _get_annotation_helper(node: Optional[QueryPlanTreeNode], step: int):
		if node is None:
			return "", step
		
		left, step = QueryPlanTree._get_annotation_helper(node.left, step)
		right, step = QueryPlanTree._get_annotation_helper(node.right, step)

		return f"{left}{right}\n{step}. Perform {node.info['Node Type']}", step + 1
	
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

		node = QueryPlanTreeNode(info, left, right, involving_relations)
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
		
		left = QueryPlanTree._get_visualization_helper(node.left, level + 1)
		if left != "":
			left = "\n" + left

		right = QueryPlanTree._get_visualization_helper(node.right, level + 1)
		if right != "":
			right = "\n" + right 

		return f"{'    ' * level}-> {node.get_primary_info()}{left}{right}"

#execute query and fetch qep
def query_plan(query: str, cursor: psycopg.Cursor) -> dict:
	cursor.execute(f"EXPLAIN (FORMAT JSON) {query}")
	return cursor.fetchone()[0][0]["Plan"]

#get aqps
def alternative_query_plans(query: str, cursor: psycopg.Cursor):
	for disabled_scan_types, disabled_join_types in product(
		combinations(SCAN_TYPE_FLAGS, len(SCAN_TYPE_FLAGS) - 1),
		combinations(JOIN_TYPE_FLAGS, len(JOIN_TYPE_FLAGS) - 1),
	):
		# Enforce single scan type and single join type
		for disabled_type in disabled_scan_types + disabled_join_types:
			cursor.execute(f"SET LOCAL enable_{disabled_type}=false")

		yield query_plan(query, cursor)
		cursor.connection.rollback()

def alternative_query_plan_trees(query: str, cursor: psycopg.Cursor):
	for plan in alternative_query_plans(query, cursor):
		yield QueryPlanTree.from_plan(plan)

def collect_scans(trees:list[QueryPlanTree])->dict[str,dict[str,float]]:
	scans={}
	
	for tree in trees:
		nodes_info=tree.root.get_all_nodes_info()
		for node in nodes_info:
			if "Scan" in node["Node Type"] and node["Node Type"] != "Bitmap Index Scan":
				if node["Relation Name"]+" "+node["Alias"] not in scans:
					scans[node["Relation Name"]+" "+node["Alias"]]={}
				scans[node["Relation Name"]+" "+node["Alias"]][node["Node Type"]]=node["Total Cost"]-node["Startup Cost"]
	return scans
