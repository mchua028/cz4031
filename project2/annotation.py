import math
from itertools import combinations, product
from typing import Optional
import graphviz

import psycopg

from preprocessing import (JOIN_TYPES, SCAN_TYPES, QueryPlanTree,
                           QueryPlanTreeNode, query_plan)

SCAN_TYPE_FLAGS = {"bitmapscan", "indexscan", "indexonlyscan", "seqscan", "tidscan"}
JOIN_TYPE_FLAGS ={"hashjoin", "mergejoin", "nestloop"}

def get_annotation(qptree: QueryPlanTree, cursor:psycopg.Cursor):
    aqps: list[QueryPlanTree] = list(alternative_query_plan_trees(qptree.query, cursor))
    scans_from_aqps: dict[str,dict[str,float]] = collect_scans_from_aqp_trees(aqps)
    joins_from_aqps: dict[str,dict[str,float]] = collect_joins_from_aqp_trees(aqps)
    return _get_annotation_helper(qptree.root, 1, scans_from_aqps, joins_from_aqps)[0]+"\n\nNote: Costs shown are estimated costs rather than actual runtime costs."

def get_visualization(qptree: QueryPlanTree):
    gv = graphviz.Digraph()
    _get_visualization_helper(qptree.root, gv, 1)
    return gv.render(format="png", view=False)

def _get_annotation_helper(node: Optional[QueryPlanTreeNode], step: int, scans_from_aqps:dict[str,dict[str,float]], joins_from_aqps:dict[str,dict[str,float]]):
    if node is None:
        return "", step

    children_annotations = []
    children_steps = []
    for child in node.children:
        child_annotation, step = _get_annotation_helper(child, step, scans_from_aqps, joins_from_aqps)
        if child_annotation == "":
            continue
        children_annotations.append(child_annotation)
        children_steps.append(f"({step - 1})")

    on_annotation = ""
    reason = ""

    if node.info["Node Type"] in SCAN_TYPES and node.info["Node Type"] != "Bitmap Index Scan":
        relation = str(next(iter(node.involving_relations)))
        on_annotation += f"on {relation}."
        scan_choices = scans_from_aqps.get(relation)
        if node.info["Node Type"] == "Bitmap Heap Scan":
            chosen_scan_cost = scan_choices.get("Bitmap Scan")
        else:
            chosen_scan_cost = node.get_cost()
        for node_type, cost in scan_choices.items():
            cost_diff = abs(cost - chosen_scan_cost)
            cost_diff_ratio = cost_diff / chosen_scan_cost
            if node_type != node.info["Node Type"] and math.isclose(cost, chosen_scan_cost):
                reason += f"\n\tUsing {node_type} in AQP has similar costs as {node.info['Node Type']}. "
            elif node_type != node.info["Node Type"] and cost > chosen_scan_cost:
                reason += f"\n\tUsing {node_type} in AQP costs {round(cost_diff_ratio, 2)}x more, which will result in increased cost of {round(cost_diff, 2)}."
            elif node_type != node.info["Node Type"] and cost < chosen_scan_cost:
                reason += f"\n\tUsing {node_type} in AQP costs {round(cost_diff_ratio, 2)}x less, which will result in cost savings of {round(cost_diff, 2)}."

        if len(scan_choices) <= 1:
            reason += "\n\tThis is the only possible scan type among all AQPs. "

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
            cost_scale = round(join_cost / cur_node_join_cost, 2)
            if join_type == node.info["Node Type"]:
                continue
            if math.isclose(cost_scale, 1.0):
                reason += f"\n\tUsing {join_type} in this AQP with equal cost as {join_type} in QEP."
            elif cost_scale > 1.0:
                reason += f"\n\tUsing {join_type} in AQP costs {cost_scale}x more, which will result in increased cost of {round(join_cost-cur_node_join_cost,2)}."
            else:
                reason += f"\n\tUsing {join_type} in AQP costs {cost_scale}x less, which will result in cost savings of {round(cur_node_join_cost-join_cost,2)}."
    else:
        on_annotation = "on result(s) from " + ", ".join(children_steps)

    return "".join(children_annotations) + f"\n\n{step}. Perform {node.info['Node Type']} {on_annotation}{reason}", step + 1


def _get_visualization_helper(node: Optional[QueryPlanTreeNode], gv: graphviz.Digraph, step: int):
    if node is None:
        return "", step

    children_ids = []
    for child in node.children:
        child_label, step = _get_visualization_helper(child, gv, step)
        if child_label == "":
            continue
        gv.node(f"{step - 1}", child_label)
        children_ids.append(step - 1)

    node_type = node.info["Node Type"]
    label = f"({step}) {node_type}"
    gv.node(str(step), label)

    for child_id in children_ids:
        gv.edge(str(step), str(child_id))

    return label, step + 1

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
			if node_type == "Bitmap Index Scan":
				continue
			if node_type == "Bitmap Heap Scan":
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
