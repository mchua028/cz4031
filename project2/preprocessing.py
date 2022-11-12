from typing import Optional, Self
import psycopg
import math
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

	def get_annotation(self, cursor:psycopg.Cursor):
		aqps: list[QueryPlanTree] = list(alternative_query_plan_trees(self.query, cursor))
		scans_from_aqps: dict[str,dict[str,float]] = collect_scans_from_aqp_trees(aqps)
		joins_from_aqps: dict[str,dict[str,float]] = collect_joins_from_aqp_trees(aqps)
		return QueryPlanTree._get_annotation_helper(self.root, 1, scans_from_aqps, joins_from_aqps)[0]+"\n\nNote: Costs shown are estimated costs rather than actual runtime costs."

	@staticmethod
	def _get_annotation_helper(node: Optional[QueryPlanTreeNode], step: int, scans_from_aqps:dict[str,dict[str,float]], joins_from_aqps:dict[str,dict[str,float]]):
		if node is None:
			return "", step

		children_annotations = []
		children_steps = []
		for child in node.children:
			child_annotation, step = QueryPlanTree._get_annotation_helper(child, step, scans_from_aqps, joins_from_aqps)
			if child_annotation == "":
				continue
			children_annotations.append(child_annotation)
			children_steps.append(f"({step - 1})")

		on_annotation = ""
		reason = ""

		if node.info["Node Type"] in SCAN_TYPES and node.info["Node Type"] != "Bitmap Index Scan":
			relation=str(next(iter(node.involving_relations)))
			on_annotation += f"on {relation}."
			scan_choices=scans_from_aqps.get(relation)
			if node.info["Node Type"]=="Bitmap Heap Scan":
				chosen_scan_cost=scan_choices.get("Bitmap Scan")
			else:
				chosen_scan_cost=node.get_cost()
			for type,cost in scan_choices.items():
				if type!=node.info["Node Type"] and math.isclose(cost,chosen_scan_cost):
					reason+=f"\n\tUsing {type} in AQP has similar costs as {node.info['Node Type']} in QEP. "
				elif type!=node.info["Node Type"] and cost>chosen_scan_cost:
					reason+=f"\n\tUsing {type} in AQP costs {round((cost-chosen_scan_cost)/chosen_scan_cost,2)}x of cost in QEP (costs {round(cost-chosen_scan_cost,2)} more."
				elif type!=node.info["Node Type"] and cost<chosen_scan_cost:
					reason+=f"\n\tUsing {type} in AQP costs {round((chosen_scan_cost-cost)/chosen_scan_cost,2)}x of cost in QEP (costs {round(chosen_scan_cost-cost,2)} less."

			if len(scan_choices) <= 1:
				reason+="\n\tThis is the only possible scan type among all AQPs. "

		elif node.info["Node Type"] in JOIN_TYPES:
			relation_key = " ".join(
				sorted(map(lambda rel: str(rel), node.involving_relations))
			)
			on_annotation = "on result(s) from " + ", ".join(children_steps)
			aqp_join_types_dict = {}
			aqp_join_types_dict = joins_from_aqps.get(relation_key)
			join_type = node.info["Node Type"]
			cur_node_join_cost = node.get_cost()

			for join_type, join_cost in aqp_join_types_dict.items():
				cost_scale = round(join_cost/ cur_node_join_cost, 2)
				if join_type == node.info["Node Type"]:
					continue
				if math.isclose(cost_scale, 1.0):
					reason += f"\n\tUsing {join_type} has similar costs as {join_type} in QEP."
				elif cost_scale > 1.0:
					reason += f"\n\tUsing {join_type} in AQP costs {cost_scale}x of cost in QEP (costs {round(join_cost-cur_node_join_cost,2)} more."
				else:
					reason += f"\n\tUsing {join_type} in AQP costs {cost_scale}x of cost in QEP (costs {round(cur_node_join_cost-join_cost,2)} less."
		else:
			on_annotation = "on result(s) from " + ", ".join(children_steps)

		return "".join(children_annotations) + f"\n\n{step}. Perform {node.info['Node Type']} {on_annotation}{reason}", step + 1

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

def alternative_query_plans(query: str, cursor: psycopg.Cursor):
	# for disabled_scan_types, disabled_join_types in product(
	# 	combinations(SCAN_TYPE_FLAGS, len(SCAN_TYPE_FLAGS) - 1),
	# 	combinations(JOIN_TYPE_FLAGS, len(JOIN_TYPE_FLAGS) - 1),
	# ):
	# 	# Enforce single scan type and single join type
	# 	for disabled_type in disabled_scan_types + disabled_join_types:
	# 		cursor.execute(f"SET LOCAL enable_{disabled_type}=false")

	# 	yield query_plan(query, cursor)
	# 	cursor.connection.rollback()
	disabled_types =[]
	for i in range(1,len(SCAN_TYPE_FLAGS)):
		for j in range(1,len(JOIN_TYPE_FLAGS)):
			disabled_types.extend(product(
				combinations(SCAN_TYPE_FLAGS, len(SCAN_TYPE_FLAGS) - i),
				combinations(JOIN_TYPE_FLAGS, len(JOIN_TYPE_FLAGS) - j),
			))
	distinct_aqps=[]
	for disabled_scan_types, disabled_join_types in disabled_types:
		for disabled_type in disabled_scan_types + disabled_join_types:
			cursor.execute(f"SET LOCAL enable_{disabled_type}=false")
		aqp_plan=query_plan(query, cursor)
		if aqp_plan not in distinct_aqps:
			distinct_aqps.append(aqp_plan)
		cursor.connection.rollback()
	return distinct_aqps

def alternative_query_plan_trees(query: str, cursor: psycopg.Cursor):
	for plan in alternative_query_plans(query, cursor):
		yield QueryPlanTree.from_plan(plan, query)

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
