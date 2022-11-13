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
    return _get_annotation_helper(qptree.root, 1, scans_from_aqps, joins_from_aqps)[0]+f"\n\nTotal cost of QEP={qptree.root.info['Total Cost']}.\nMinimum total cost across all AQPs={min(map(lambda aqp: aqp.root.info['Total Cost'], aqps))}.\n\nNote: Costs shown are estimated costs rather than actual runtime costs."

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
            if chosen_scan_cost==0:
                if cost>chosen_scan_cost:
                    reason+=f"\n\tCost of using {node_type} is {round(cost-chosen_scan_cost,2)} more than the cost of using {node.info['Node Type']}."
                else:
                    reason+=f"\n\tCost of using {node_type} is similar to cost of using {node.info['Node Type']}. "
                continue
            cost_ratio = round(cost / chosen_scan_cost,2)
            if node_type==node.info["Node Type"]:
                continue
            if math.isclose(cost_ratio,1.0):
                reason+=f"\n\tCost of using {node_type} is similar to cost of using {node.info['Node Type']}. "
            elif cost_ratio>1.0:
                reason+=f"\n\tCost of using {node_type} is {cost_ratio}x the cost of using {node.info['Node Type']} (costs {round(cost-chosen_scan_cost,2)} more)."
            else:
                reason+=f"\n\tCost of using {node_type} is {cost_ratio}x the cost of using {node.info['Node Type']} (costs {round(chosen_scan_cost-cost,2)} less)."

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
            if cur_node_join_cost==0:
                if join_cost>cur_node_join_cost:
                    reason+=f"\n\tCost of using {join_type} is {round(join_cost-cur_node_join_cost,2)} more than the cost of using {node.info['Node Type']}."
                else:
                    reason+=f"\n\tCost of using {join_type} is similar to cost of using {node.info['Node Type']}. "
                continue
            cost_scale = round(join_cost / cur_node_join_cost, 2)
            if join_type == node.info["Node Type"]:
                continue
            if math.isclose(cost_scale, 1.0):
                reason += f"\n\tCost of using {join_type} is similar to cost of using {node.info['Node Type']}."
            elif cost_scale > 1.0:
                reason += f"\n\tCost of using {join_type} is {cost_scale}x the cost of using {node.info['Node Type']} (costs {round(join_cost-cur_node_join_cost,2)} more)."
            else:
                reason += f"\n\tCost of using {join_type} is {cost_scale}x the cost of using {node.info['Node Type']} (costs {round(cur_node_join_cost-join_cost,2)} less)."
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
    disabled_types =[]
    for i in range(0,len(SCAN_TYPE_FLAGS)+1):
        for j in range(0,len(JOIN_TYPE_FLAGS)+1):
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
