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
								 const std::vector<double>& flops, std::vector<double> intraNodeCommunications,
								 std::vector<double> interNodeCommunications) :
		CombinedTask(name, iterations, synchronized, flops),
		intraNodeCommunications(std::move(intraNodeCommunications)),
		interNodeCommunications(std::move(interNodeCommunications)) {}

CombinedGpuTask::CombinedGpuTask(const std::string& name, const std::string& iterations, bool synchronized,
								 const std::vector<double>& flops, const std::string& communicationModel,
								 MatrixPattern communicationPattern) :
		CombinedTask(name, iterations, synchronized, flops, communicationModel, communicationPattern) {}

CombinedGpuTask::CombinedGpuTask(const std::string& name, const std::string& iterations, bool synchronized,
								 const std::string& computationModel, VectorPattern computationPattern,
								 std::vector<double> intraNodeCommunications,
								 std::vector<double> interNodeCommunications) :
		CombinedTask(name, iterations, synchronized, computationModel, computationPattern),
		intraNodeCommunications(std::move(intraNodeCommunications)),
		interNodeCommunications(std::move(interNodeCommunications)) {}

CombinedGpuTask::CombinedGpuTask(const std::string& name, const std::string& iterations, bool synchronized,
								 const std::string& computationModel, VectorPattern computationPattern,
								 const std::string& communicationModel,
								 MatrixPattern communicationPattern) :
		CombinedTask(name, iterations, synchronized, computationModel, computationPattern, communicationModel,
					 communicationPattern) {}

void CombinedGpuTask::execute(const Node* node, const Job* job,
							  const std::vector<Node*>& nodes, int rank,
							  simgrid::s4u::BarrierPtr barrier) const {

	std::vector<s4u_Mailbox*> gpuCallbacks;
	s4u_Mailbox* gpuLinkCallback = s4u_Mailbox::by_name("GPULink@" + node->getHostName());
	int numGpusPerNode = job->getExecutingNumGpusPerNode();
	std::vector<Gpu*> gpus = node->getGpus();
	if (numGpusPerNode == 0) {
		xbt_die("GPU task not executable: no GPUs assigned");
	}
	if (numGpusPerNode > gpus.size()) {
		xbt_die("Number of required GPUs (%d) higher than number of GPUs on node (%zu)", numGpusPerNode, gpus.size());
	}

	if (!flops.empty() && flops[rank] > 0) {
		double flopsPerGpu = flops[rank] / numGpusPerNode;
		if (numGpusPerNode == 1) {
			XBT_INFO("Processing %f FLOPS on one GPU", flops[rank]);
		} else {
			XBT_INFO("Processing %f FLOPS on %u GPUs (%f each)", flops[rank], numGpusPerNode, flopsPerGpu);
		}
		std::vector<Gpu*> gpuCandidates;
		std::vector<Gpu*> allocatedGpus;
		for (auto& gpu: gpus) {
			if (gpu->getState() == GPU_FREE) {
				gpuCandidates.push_back(gpu);
			} else {
				allocatedGpus.push_back(gpu);
			}
		}
		gpuCandidates.insert(std::end(gpuCandidates), std::begin(allocatedGpus), std::end(allocatedGpus));
		for (int i = 0; i < numGpusPerNode; ++i) {
			gpuCallbacks.push_back(gpuCandidates[i]->execAsync(flopsPerGpu));
		}
	}

	if (!intraNodeCommunications.empty()) {
		gpuLinkCallback = node->execGpuTransferAsync(intraNodeCommunications, numGpusPerNode);
	}

	if (!interNodeCommunications.empty()) {
		size_t numberOfAssignedNodes = nodes.size();
		int destinationRank = 0;
		for (auto& assignedNode: nodes) {
			int index = rank * numberOfAssignedNodes + destinationRank++;
			if (interNodeCommunications[index] > 0) {
				XBT_INFO("Sending %f bytes to %s", interNodeCommunications[index], assignedNode->getHostName().c_str());
			}
		}
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
	for (auto& gpuCallback: gpuCallbacks) {
		gpuCallback->get<void>();
	}
	if (!intraNodeCommunications.empty()) {
		gpuLinkCallback->get<void>();
	}
}

void CombinedGpuTask::scaleTo(int numNodes, int numGpusPerNode) {
	CombinedTask::scaleTo(numNodes, numGpusPerNode);
	if (!communicationModel.empty()) {
		std::tie(intraNodeCommunications, interNodeCommunications) =
				Utility::createMatrices(communicationModel, communicationPattern, numNodes, numGpusPerNode);
	}
}
