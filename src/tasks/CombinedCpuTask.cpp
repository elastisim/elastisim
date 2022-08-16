/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "CombinedCpuTask.h"

#include <utility>

#include "Node.h"
#include "Utility.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(CombinedCpuTask, "Messages within the combined CPU task");

CombinedCpuTask::CombinedCpuTask(const std::string& name, const std::string& iterations, bool synchronized,
								 const std::vector<double>& flops, std::vector<double> payloads, bool coupled) :
		CombinedTask(name, iterations, synchronized, flops), payloads(std::move(payloads)), coupled(coupled) {}

CombinedCpuTask::CombinedCpuTask(const std::string& name, const std::string& iterations, bool synchronized,
								 const std::vector<double>& flops, const std::string& communicationModel,
								 MatrixPattern communicationPattern, bool coupled) :
		CombinedTask(name, iterations, synchronized, flops, communicationModel, communicationPattern), coupled(coupled) {}

CombinedCpuTask::CombinedCpuTask(const std::string& name, const std::string& iterations, bool synchronized,
								 const std::string& computationModel, VectorPattern computationPattern,
								 std::vector<double> payloads, bool coupled) :
		CombinedTask(name, iterations, synchronized, computationModel, computationPattern), payloads(std::move(payloads)),
		coupled(coupled) {}

CombinedCpuTask::CombinedCpuTask(const std::string& name, const std::string& iterations, bool synchronized,
								 const std::string& computationModel, VectorPattern computationPattern,
								 const std::string& communicationModel, MatrixPattern communicationPattern,
								 bool coupled) :
		CombinedTask(name, iterations, synchronized, computationModel, computationPattern, communicationModel,
					 communicationPattern), coupled(coupled) {}

void CombinedCpuTask::execute(const Node* node, const Job* job,
							  const std::vector<Node*>& nodes, int rank,
							  simgrid::s4u::BarrierPtr barrier) const {
	if (coupled && !flops.empty() && !payloads.empty()) {
		if (rank == 0) {
			std::vector<simgrid::s4u::Host*> hosts;
			std::vector<Node*> assignedNodes = nodes;
			auto func = [](const Node* node) { return node->getHost(); };
			std::transform(std::begin(assignedNodes), std::end(assignedNodes), std::back_inserter(hosts), func);
			simgrid::s4u::this_actor::parallel_execute(hosts, flops, payloads);
		}
		barrier->wait();
	} else {
		std::vector<simgrid::s4u::ActivityPtr> activities;
		if (!flops.empty() && flops[rank] > 0) {
			XBT_INFO("Processing %f FLOPS", flops[rank]);
			activities.emplace_back(node->getHost()->exec_async(flops[rank]));
		}
		if (!payloads.empty()) {
			int numberOfAssignedNodes = nodes.size();
			int destinationRank = 0;
			for (auto& assignedNode: nodes) {
				int index = rank * numberOfAssignedNodes + destinationRank++;
				if (payloads[index] > 0) {
					XBT_INFO("Sending %f bytes to %s", payloads[index], assignedNode->getHostName().c_str());
				}
			}
			if (rank == 0) {
				std::vector<simgrid::s4u::Host*> hosts;
				std::vector<Node*> assignedNodes = nodes;
				auto func = [](const Node* node) { return node->getHost(); };
				std::transform(std::begin(assignedNodes), std::end(assignedNodes), std::back_inserter(hosts), func);
				std::vector<double> empty(numberOfAssignedNodes);
				simgrid::s4u::this_actor::parallel_execute(hosts, empty, payloads);
			}
			barrier->wait();
		}
		for (auto& activity: activities) {
			activity->wait();
		}
	}
}

void CombinedCpuTask::scaleTo(int numNodes, int numGpusPerNode) {
	CombinedTask::scaleTo(numNodes, numGpusPerNode);
	if (!communicationModel.empty()) {
		payloads = Utility::createMatrix(communicationModel, communicationPattern, numNodes, numGpusPerNode);
	}
}
