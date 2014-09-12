/**
 * @file BlameTreeAnalysis.cpp
 * @brief
 */

/*
 * Copyright (c) 2013, UC Berkeley All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1.  Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the UC Berkeley nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY UC BERKELEY ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL UC BERKELEY BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

// Author: Cuong Nguyen

#include "BlameTreeAnalysis.h"

#include <queue>
#include <vector>
#include <map>

using std::queue;
using std::map;
using std::vector;

// TODO: document what "52" is
const BlameNode& BlameTreeAnalysis::constructFuncBlameNode(const BlameTreeShadowObject<HIGHPRECISION>& left,
		PRECISION precision,
		const BlameTreeShadowObject<HIGHPRECISION>& right) {
	int dpc = left.getDPC();
	BlameNodeID bnID(dpc, precision);
	auto it = nodes.find(bnID);

	//
	// nodes remember all nodes that are processed before
	// process again only if this is not processed before
	//
	if (it != nodes.end()) {
		return it->second;
	}

	//
	// variables definition
	//
	int pc = left.getPC();
	int fid = left.getFileID();
	HIGHPRECISION value = left.getValue(precision);
	string func = left.getFunc();

	//
	// construct a node associate with the function result
	//
	BlameNode node(dpc, pc, fid, false, precision, {}, {});

	int dpc01 = right.getDPC();

	//
	// try different combination of precision of argument
	//
	for (PRECISION i = BITS_FLOAT; i < PRECISION_NO; i = PRECISION(i + 1)) {
		HIGHPRECISION value01 = right.getValue(i);
		HIGHPRECISION lowvalue01 = right.getValue(BITS_FLOAT);

		safe_assert(value == BlameTreeUtilities::clearBits(value, 52 - BlameTreeUtilities::exactBits(precision)));
		if (!BlameTreeUtilities::equalWithPrecision(value, BlameTreeUtilities::evalFunc(value01, func), precision)) {
			continue;
		}
		/*
		if (BlameTreeUtilities::clearBits(value, 52 -
		BlameTreeUtilities::exactBits(precision)) ==
		BlameTreeUtilities::clearBits(BlameTreeUtilities::evalFunc(value01,
		func), 52 - BlameTreeUtilities::exactBits(precision)))
		*/
		//
		// Construct edges for each blame
		//
		BlameNodeID bnID01(dpc01, i);
		vector<BlameNodeID> blamedNodes;

		//
		// Blame only if the operand cannot be in the lowest precision (right
		// now BITS_23)
		//
		if (i != BITS_FLOAT &&
				BlameTreeUtilities::clearBits(lowvalue01, 52 - BlameTreeUtilities::exactBits(i)) != value01) {
			blamedNodes.push_back(bnID01);
		}

		node.edges.push_back(blamedNodes);
		node.edgeAttributes.push_back(false);

		//
		// Do not try larger i because it subsumes what have been tried
		//
		break;
	}

	//
	// Determined whether node is highlighted
	//
	if (value != (LOWPRECISION)value) {
		node.highlight = true;
	}

	nodes[bnID] = node;
	return nodes[bnID];
}


const BlameNode& BlameTreeAnalysis::constructBlameNode(const BlameTreeShadowObject<HIGHPRECISION>& left,
		PRECISION precision,
		const BlameTreeShadowObject<HIGHPRECISION>& right01,
		const BlameTreeShadowObject<HIGHPRECISION>& right02) {

	int dpc = left.getDPC();
	BlameNodeID bnID(dpc, precision);
	auto it = nodes.find(bnID);

	//
	// nodes remember all nodes that are processed before
	// process again only if this is not processed before
	//
	if (it != nodes.end()) {
		return it->second;
	}

	//
	// variables definition
	//
	int pc = left.getPC();
	int fid = left.getFileID();
	HIGHPRECISION value = left.getValue(precision);
	BINOP bop = left.getBinOp();

	//
	// construct a node associate with the binary operation result
	//
	BlameNode node(dpc, pc, fid, false, precision, {}, {});

	int dpc01 = right01.getDPC();
	int dpc02 = right02.getDPC();

	//
	// try different combination of precision of the two operands
	//
	PRECISION max_j = PRECISION_NO;
	for (PRECISION i = BITS_FLOAT; i < PRECISION_NO; i = PRECISION(i + 1)) {
		HIGHPRECISION value01 = right01.getValue(i);
		HIGHPRECISION lowvalue01 = right01.getValue(BITS_FLOAT);

		PRECISION j = BITS_FLOAT;
		for (; j < max_j; j = PRECISION(j + 1)) {
			HIGHPRECISION value02 = right02.getValue(j);
			HIGHPRECISION lowvalue02 = right02.getValue(BITS_FLOAT);

			safe_assert(value == BlameTreeUtilities::clearBits(value, 52 - BlameTreeUtilities::exactBits(precision)));
			if (!BlameTreeUtilities::equalWithPrecision(value, BlameTreeUtilities::eval(value01, value02, bop), precision)) {
				continue;
			}
			/*
			if (value ==
			BlameTreeUtilities::clearBits(BlameTreeUtilities::eval(value01,
			    value02, bop), 52 - BlameTreeUtilities::exactBits(precision)))
			{*/
			//
			// Construct edges for each blame
			//
			BlameNodeID bnID01(dpc01, i);
			BlameNodeID bnID02(dpc02, j);


			//
			// Blame only if the operand cannot be in the lowest precision (right
			// now BITS_23)
			//
			vector<BlameNodeID> blamedNodes;
			if (i != BITS_FLOAT &&
					BlameTreeUtilities::clearBits(lowvalue01, 52 - BlameTreeUtilities::exactBits(i)) != value01) {
				blamedNodes.push_back(bnID01);
			}

			if (j != BITS_FLOAT &&
					BlameTreeUtilities::clearBits(lowvalue02, 52 - BlameTreeUtilities::exactBits(j)) != value02) {
				blamedNodes.push_back(bnID02);
			}

			node.edges.push_back(blamedNodes);

			//
			// Determine the edge attribute
			//
			node.edgeAttributes.push_back(
				!BlameTreeUtilities::equalWithPrecision(value, BlameTreeUtilities::feval(value01, value02, bop), precision));
			/*
			node.addEdgeAttribute(BlameTreeUtilities::clearBits(value, 52 -
			      BlameTreeUtilities::exactBits(precision)) !=
			    BlameTreeUtilities::clearBits(BlameTreeUtilities::feval(value01,
			        value02, bop), 52 -
			      BlameTreeUtilities::exactBits(precision)));
			      */

			//
			// Do not try larger j because it subsumes what have been tried
			//
			break;
		}
		//
		// Do not try larger j because it subsumes what have been tried
		//
		max_j = j;
	}

	//
	// Determined whether node is highlighted
	//
	if (value != (LOWPRECISION)value) {
		node.highlight = true;
	}

	nodes[bnID] = node;
	return nodes[bnID];
}

// TODO: FIXME: we implicitedly assume trace[dpc] is a non-empty vector, which is bad...
const BlameNode&
BlameTreeAnalysis::constructBlameGraph(const map<int, vector<BlameTreeShadowObject<HIGHPRECISION>>>& trace) {

	queue<BlameNodeID> workList = queue<BlameNodeID>({rootNode});

	while (!workList.empty()) {
		//
		// Variable definitions.
		//
		BlameNodeID bnID = workList.front();
		workList.pop();
		if (nodes.find(bnID) != nodes.end()) {
			continue;
		}

		int dpc = bnID.dpc;
		PRECISION precision = bnID.precision;
		auto it = trace.find(dpc);
		if (it == trace.end()) {
			safe_assert(false);
		}
		const auto& startNode = it->second;

		//
		// We are assuming that each element of the trace has three elements.
		// Construct blameGraph given the start node. Recursively construct
		// blameGraph for all operands that are blamed by the start node.
		//
		BlameNode blameGraph;
		switch (startNode[0].getIntrType()) {
			case BIN_INTR:
				safe_assert(startNode.size() == 3);
				blameGraph = constructBlameNode(startNode[0], precision, startNode[1], startNode[2]);
				break;
			case CALL_INTR:
				safe_assert(startNode.size() == 2);
				blameGraph = constructFuncBlameNode(startNode[0], precision, startNode[1]);
				break;
			default:
				DEBUG_STDERR("Unsupport kind of instruction.");
				safe_assert(false);
		}

		const vector<vector<BlameNodeID>>& blameEdges = blameGraph.edges;

		// TODO: consider unordered_set here
		set<BlameNodeID> nodeIds;
		for (const auto& blameNodes : blameEdges) {
			for (const auto& blameNode : blameNodes) {
				//
				// Construct graph for this node if it is never constructed before
				//
				nodeIds.insert(blameNode);
			}
		}

		for (const auto& blameNode : nodeIds) {
			if (nodes.find(blameNode) == nodes.end()) {
				workList.push(blameNode);
			}
		}
	}

	return nodes[rootNode];
}

std::string BlameTreeAnalysis::edgeToDot(const BlameNode& graph) const {
	std::ostringstream dot;
	dot << "\t" << graph.edgeToDot(nodes) << endl;
	return dot.str();
}

std::string BlameTreeAnalysis::toDot() const {
	std::ostringstream dot;
	dot << "digraph G { " << endl;

	for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
		const BlameNode& bn = it->second;

		if (bn.highlight) {
			dot << "\t" << bn.toDot() << "[color=red]" << endl;
		}

		dot << edgeToDot(bn);
	}

	dot << "}" << endl;

	return dot.str();
}

struct location {
	int file;
	int pc;
	location(int f, int p) : file(f), pc(p) {}
	bool operator<(const location& rhs) const {
		if (file == rhs.file) {
			return pc < rhs.pc;
		}
		return file < rhs.file;
	}
};

struct highlighting {
	bool highlight;
	bool edgeHighlight;
	highlighting(bool h = false, bool eh = false) : highlight(h), edgeHighlight(eh) {}
};

highlighting operator||(const highlighting& lhs, const highlighting& rhs) {
	return highlighting(lhs.highlight || rhs.highlight, lhs.edgeHighlight || rhs.edgeHighlight);
}

void BlameTreeAnalysis::printResult() const {

	// set of already considered blame nodes
	map<location, highlighting> result;  // map from pc to a pair of boolean, the first
	// boolean indicates whether the result
	// requires higherprecision, the second
	// boolean indicates whether the operator
	// requires higher precision


	// TODO: consider whether this should be an unordered_set
	set<BlameNodeID> cacheNodes = {rootNode};
	queue<BlameNodeID> workList = queue<BlameNodeID>({rootNode});

	while (!workList.empty()) {
		BlameNodeID bnID = workList.front();
		workList.pop();

		auto it = nodes.find(bnID);
		if (it == nodes.end()) {
			continue;
		}
		BlameNode bn = it->second;

		int pc = bn.dpc;
		int file = bn.fid;
		bool highlight = bn.highlight;
		const vector<vector<BlameNodeID>>& edges = bn.edges;

		if (edges.empty()) {
			continue;
		}

		const vector<bool>& edgeAttributes = bn.edgeAttributes;
		bool edgeHighlight = edgeAttributes[0];

		//
		// save the result for the current node
		//
		if (result.find(location(file, pc)) == result.end()) {
			result[location(file, pc)] = highlighting(highlight, edgeHighlight);
		} else {
			location loc = location(file, pc);
			result[loc] = result[loc] || highlighting(highlight, edgeHighlight);
		}

		const vector<BlameNodeID>& bnIDs = edges[0];
		for (const auto bnID : bnIDs) {
			if (cacheNodes.find(bnID) == cacheNodes.end()) {
				cacheNodes.insert(bnID);
				workList.push(bnID);
			}
		}
	}

	//
	// print result
	//
	for (const auto& result_p : result) {
		int file = result_p.first.file;
		bool pc = result_p.first.pc;
		bool highlight = result_p.second.highlight;
		bool edgeHighlight = result_p.second.edgeHighlight;

		if (highlight || edgeHighlight) {
			cout << "\t File: " << file << ", Line " << pc << ":" << endl;
			if (highlight) {
				cout << "\t\t Result: double precision" << endl;
			}
			if (edgeHighlight) {
				cout << "\t\t Operator: double precision" << endl;
			}
		}
	}
}
