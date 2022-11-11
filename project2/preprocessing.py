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

	def get_cost(self) -> float:
		cost: float = self.info["Total Cost"]
		if self.right is not None:
			cost -= self.right.info["Total Cost"]
		if self.left is not None:
			cost -= self.left.info["Total Cost"]
		return round(cost, 1)

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

	def get_annotation(self,query: str,cursor:psycopg.Cursor):
		aqps:list[QueryPlanTree] = alternative_query_plan_trees(query,cursor)
		scans_from_aqps:dict[str,dict[str,float]] = collect_scans_from_aqp_trees(aqps)
		# generator objects can only be iterated over once, therefore we will need to reinitialise
		# aqps in order to collect join information
		# See: https://stackoverflow.com/questions/231767/what-does-the-yield-keyword-do-in-python
		aqps:list[QueryPlanTree] = alternative_query_plan_trees(query,cursor)
		joins_from_aqps:dict[str,dict[str,float]] = collect_joins_from_aqp_trees(aqps)
		return QueryPlanTree._get_annotation_helper(self.root, 1, scans_from_aqps, joins_from_aqps)[0]

	@staticmethod
	def _get_annotation_helper(node: Optional[QueryPlanTreeNode], step: int, scans_from_aqps:dict[str,dict[str,float]], joins_from_aqps:dict[str,dict[str,float]]):
		if node is None:
			return "", step

		left_annotation, step = QueryPlanTree._get_annotation_helper(node.left, step,scans_from_aqps, joins_from_aqps)
		right_annotation, step = QueryPlanTree._get_annotation_helper(node.right, step,scans_from_aqps, joins_from_aqps)

		on_annotation = ""
		reason = ""

		if node.info["Node Type"] in SCAN_TYPES and node.info["Node Type"] != "Bitmap Index Scan":
			relation=str(next(iter(node.involving_relations)))
			on_annotation += f"on {relation}."
			more_costly_scans:dict[str,float] = {}
			less_costly_scans:dict[str,float] = {}
			print(relation)
			scan_choices=scans_from_aqps.get(relation)
			print(scan_choices)
			if node.info["Node Type"]=="Bitmap Heap Scan":
				chosen_scan_cost=scan_choices.get("Bitmap Scan")
			else:
				chosen_scan_cost=scan_choices.get(node.info["Node Type"])
			for type,cost in scan_choices.items():
				if type!=node.info["Node Type"] and cost>chosen_scan_cost:
					more_costly_scans[type]=round((cost-chosen_scan_cost)/chosen_scan_cost,2)
				elif type!=node.info["Node Type"] and cost<chosen_scan_cost:
					more_costly_scans[type]=round((chosen_scan_cost-cost)/chosen_scan_cost,2)

			if len(more_costly_scans)!=0:
				for type,cost in more_costly_scans.items():
					reason+=f"\n\tUsing {type} costs {cost}x more. "
			elif not(len(scan_choices)>1):
				reason+="\n\tThis is the only possible scan type among all AQPs. "
			else:
				for type,cost in less_costly_scans.items():
					reason+=f"\n\tUsing {type} costs {cost}x less. "
				reason+="These cost savings are negligible. "
		elif node.info["Node Type"] in JOIN_TYPES:
			relation_key = " ".join(
				sorted(map(lambda rel: str(rel), node.involving_relations))
			)
			on_annotation = f"on {relation_key},"
			aqp_join_types_dict = {}
			if relation_key not in joins_from_aqps:
				reason += f"This is the only join type performed on {relation_key} among all AQPs."
			else:
				aqp_join_types_dict = joins_from_aqps.get(relation_key)
				join_type = node.info["Node Type"]
				cur_node_join_cost = node.get_cost()
				
				for join_type, join_cost in aqp_join_types_dict.items():
					cost_scale = round(join_cost/ cur_node_join_cost, 2)
					if cost_scale > 1.0:
						reason += f"\n\tUsing {join_type} in AQP costs {cost_scale}x more."
					elif cost_scale == 1.0:
						reason += f"\n\tUsing {join_type} in this AQP with equal cost as {join_type} in QEP."
					else:
						reason += f"\n\tUsing {join_type} in AQP costs {cost_scale}x less."
			

		
		return f"{left_annotation}{right_annotation}\n{step}. Perform {node.info['Node Type']} {on_annotation}{reason}", step + 1

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

def query_plan(query: str, cursor: psycopg.Cursor) -> dict:
	cursor.execute(f"EXPLAIN (FORMAT JSON) {query}")
	return cursor.fetchone()[0][0]["Plan"]

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

def collect_joins_from_aqp_trees(aqp_trees: list[QueryPlanTree]) -> dict[str, dict[str, float]]:
	result = {}
	for tree in aqp_trees:
		for node in tree.join_nodes:
			relations_key = " ".join(
				# Sorting is required as the relations may not be in order
				sorted(map(lambda rel: str(rel), node.involving_relations))
			)
			if relations_key not in result:
				result[relations_key] = {}

			node_type = node.info["Node Type"]
			cost = node.get_cost()
			if node_type not in result[relations_key] or cost < result[relations_key][node_type]:
				result[relations_key][node_type] = cost
	return result

def collect_scans_from_aqp_trees(aqp_trees: list[QueryPlanTree]) -> dict[str, dict[str, float]]:
	result = {}
	for tree in aqp_trees:
		for node in tree.scan_nodes:
			node_type = node.info["Node Type"]
			if node_type=="Bitmap Index Scan":
				continue
			if node_type=="Bitmap Heap Scan":
				node_type = "Bitmap Scan"
			relations_key = " ".join(
				# Sorting is required as the relations may not be in order
				sorted(map(lambda rel: str(rel), node.involving_relations))
			)

			if relations_key not in result:
				result[relations_key] = {}
			cost = node.get_cost()
			if node_type not in result[relations_key] or cost < result[relations_key][node_type]:
				result[relations_key][node_type] = cost

	return result
