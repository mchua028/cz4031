from typing import List, Optional, Self
import json
import psycopg

# Assume always binary tree
class QueryPlanTreeNode:
	def __init__(self, info: dict, desc: Optional[str]="", left: Optional[Self] = None, right: Optional[Self] = None):
		self.info = info
		self.left = left
		self.right = right
		self.desc = desc

	def get_annotation(self) -> None: 
		types=['Hash Cond', 'Cond'] #unsure if there are more types of filtering keys
		for k in self.info.items():
			if k[0] == "Node Type":
				self.desc += "Perform "+k[1]
			elif k[0] == "Relation Name":
				self.desc += " on table "+(k[1].upper())
			elif k[0] == "Alias":
				self.desc += " as "+k[1]
			elif k[0] in types:
				self.desc += " and filtering on "+k[1]
		
		

	def get_primary_info(self) -> dict:
		# List of node types -> https://github.com/postgres/postgres/blob/master/src/backend/commands/explain.c#L1191
		primary_info = {}
		for k, v in self.info.items():
			if k == "Node Type" or k == "Relation Name" \
				or k == "Alias" or k == "Group Key" or k == "Strategy" \
				or ("Filter" in k and "Removed by" not in k) or "Cond" in k:
				primary_info[k] = v
		self.get_annotation()
		#print(self.desc)
		return primary_info
		


	
	
	

class QueryPlanTree:
	root: Optional[QueryPlanTreeNode]

	def __init__(self):
		self.root = None
	
	def postOrder(root) -> str:
		if root==None:
			return ""
        #traverse left subtree
		QueryPlanTree.postOrder(root.left)
        #traverse right subtree
		QueryPlanTree.postOrder(root.right)
        #traverse root
		#print(root.info)
		#print(root.desc)
	
	def __init__(self, cursor: psycopg.Cursor, query: str):
		plan: dict = explain_query(cursor, query)["Plan"]
		self.root = self._build(plan)
		print(plan)
		QueryPlanTree.postOrder(self.root)
	
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
		annotated, step = QueryPlanTree.gen_annotation(self.root, 1) 
		return annotated

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
		#del node.info["Node Type"]

		return f"{'    ' * level}-> {node_type} {node.get_primary_info()}{left}{right}"

	@staticmethod
	def gen_annotation(node: Optional[QueryPlanTreeNode], step:int):
		if node is None:
			return ("", step)
		
		left, step = QueryPlanTree.gen_annotation(node.left, step)

		right, step = QueryPlanTree.gen_annotation(node.right, step)
		
		if("Join Type" in node.info.keys() or "Relation Name" in node.info.keys()):
			if("Relation Name" in node.info.keys()):
				node_type = node.info.get("Node Type")
				rela_name = node.info.get("Relation Name")
				total_cost = node.info.get("Total Cost")
				return (f"{left}{right} Step {step}: Perform {node_type} on {rela_name} with cost: {total_cost}\n", step+1)

			else:
				node_type = node.info.get("Node Type")
				total_cost = node.info.get("Total Cost")
				rela_name1 = "1"
				rela_name2 = "2"
				return (f"{left}{right} Step {step}: Perform {node_type} on {rela_name1} and {rela_name2} with cost: {total_cost}\n", step+1)

		else:
			return(f"{left}{right}", step)

	
		
def explain_query(cursor: psycopg.Cursor, query: str, debug: bool = False) -> dict:
	cursor.execute(
		f"EXPLAIN (FORMAT JSON, ANALYZE) {query}"
	)
	query_explanation = cursor.fetchone()[0][0]
	if debug:
		print("Query Explanation:", json.dumps(query_explanation, indent=2))
	
	return query_explanation