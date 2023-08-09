/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "CombinedGpuTask.h"
#include "Node.h"
#include "Gpu.h"
#include "Job.h"
#include "Utility.h"
#include <simgrid/s4u.hpp>
#include <utility>

XBT_LOG_NEW_DEFAULT_CATEGORY(CombinedGpuTask, "Messages within the combined GPU task");

CombinedGpuTask::CombinedGpuTask(const std::string& name, const std::string& iterations, bool synchronized,
								 const std::optional<std::vector<double>>& flops,
								 const std::optional<std::string>& computationModel, VectorPattern computationPattern,
								 const std::optional<std::string>& communicationModel,
								 MatrixPattern communicationPattern,
								 std::optional<std::vector<double>> intraNodeCommunications,
								 std::optional<std::vector<double>> interNodeCommunications) :
		CombinedTask(name, iterations, synchronized, flops, computationModel, computationPattern, communicationModel,
					 communicationPattern),
		intraNodeCommunications(intraNodeCommunications.has_value() ? std::move(intraNodeCommunications.value())
																	: std::vector<double>()),
		interNodeCommunications(interNodeCommunications.has_value() ? std::move(interNodeCommunications.value())
																	: std::vector<double>()) {
	if (intraNodeCommunications.has_value() != interNodeCommunications.has_value()) {
		xbt_die("Specifying only one of intra- or inter-node communication is invalid.");
	}
}

void CombinedGpuTask::execute(const Node* node, const Job* job, const std::vector<Node*>& nodes, int rank,
							  simgrid::s4u::BarrierPtr barrier) const {

	std::vector<s4u_Mailbox*> gpuCallbacks;
	s4u_Mailbox* gpuLinkCallback = s4u_Mailbox::by_name("GPULink@" + node->getHostName());
	int numGpusPerNode = job->getExecutingNumGpusPerNode();
	std::vector<const Gpu*> gpus = node->getGpus();
	if (numGpusPerNode == 0) {
		xbt_die("GPU task not executable: no GPUs assigned");
	}
	if (numGpusPerNode > gpus.size()) {
		xbt_die("Number of required GPUs (%d) higher than number of GPUs on node (%zu)", numGpusPerNode, gpus.size());
	}

	if (!flops.empty() && flops[rank] > 0) {
		double flopsPerGpu = flops[rank] / numGpusPerNode;
		gpuCallbacks = node->execGpuComputationAsync(numGpusPerNode, flopsPerGpu);
	}

	if (!intraNodeCommunications.empty()) {
		gpuLinkCallback = node->execGpuTransferAsync(intraNodeCommunications, numGpusPerNode);
	}

	if (!interNodeCommunications.empty()) {
		size_t numberOfAssignedNodes = nodes.size();
		int destinationRank = 0;
		for (const auto& assignedNode: nodes) {
			int index = rank * numberOfAssignedNodes + destinationRank++;
			if (interNodeCommunications[index] > 0) {
				XBT_INFO("Sending %f bytes to %s", interNodeCommunications[index], assignedNode->getHostName().c_str());
			}
		}
		barrier->wait();
		if (rank == 0) {
			std::vector<s4u_Host*> hosts;
			std::vector<Node*> assignedNodes = nodes;
			auto func = [](const Node* node) { return node->getHost(); };
			std::transform(std::begin(assignedNodes), std::end(assignedNodes), std::back_inserter(hosts), func);
			std::vector<double> empty(numberOfAssignedNodes);
			simgrid::s4u::this_actor::parallel_execute(hosts, empty, interNodeCommunications);
		}
		barrier->wait();
	}
	for (const auto& gpuCallback: gpuCallbacks) {
		gpuCallback->get<void>();
	}
	if (!intraNodeCommunications.empty()) {
		gpuLinkCallback->get<void>();
	}
}

void
CombinedGpuTask::scaleTo(int numNodes, int numGpusPerNode, const std::map<std::string, std::string>& runtimeArguments) {
	CombinedTask::scaleTo(numNodes, numGpusPerNode, runtimeArguments);
	if (!communicationModel.empty()) {
		std::tie(intraNodeCommunications, interNodeCommunications) =
				Utility::createMatrices(communicationModel, communicationPattern, numNodes, numGpusPerNode,
										runtimeArguments);
	}
}
