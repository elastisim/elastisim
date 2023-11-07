/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "BurstBufferWriteTask.h"
#include "Node.h"
#include "Job.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(BurstBufferWriteTask, "Messages within the burst buffer write task");

BurstBufferWriteTask::BurstBufferWriteTask(const std::string& name, const std::string& iterations, bool synchronized,
										   bool asynchronous, const std::optional<std::vector<double>>& ioSizes,
										   const std::optional<std::string>& ioModel, VectorPattern ioPattern) :
		IoTask(name, iterations, synchronized, asynchronous, ioSizes, ioModel, ioPattern) {}

void BurstBufferWriteTask::execute(const Node* node, const Job* job, const std::vector<Node*>& nodes, int rank,
								   simgrid::s4u::BarrierPtr barrier) const {
	std::vector<simgrid::s4u::ActivityPtr> activities = executeAsync(node, job, nodes, rank);
	for (const auto& activity: activities) {
		activity->wait();
	}
}

std::vector<simgrid::s4u::ActivityPtr>
BurstBufferWriteTask::executeAsync(const Node* node, const Job* job, const std::vector<Node*>& nodes, int rank) const {
	if (node->getType() == COMPUTE_NODE_WITH_BB) {
		XBT_INFO("Writing %f bytes to burst buffer", ioSizes[rank]);
		return {node->getNodeLocalBurstBuffer()->write_async(ioSizes[rank])};
	} else if (node->getType() == COMPUTE_NODE_WITH_WIDE_STRIPED_BB) {
		XBT_INFO("Writing %f bytes to wide-striped burst buffers", ioSizes[rank]);
		std::vector<simgrid::s4u::ActivityPtr> activities;
		int numNodes = nodes.size();
		double sizePerHost = ioSizes[rank] / numNodes;
		std::vector<simgrid::s4u::Host*> hosts;
		std::vector<double> flops;
		std::vector<double> payloads(numNodes * numNodes);
		int nodeRank = 0;
		for (const auto& assignedNode: nodes) {
			hosts.emplace_back(assignedNode->getHost());
			flops.emplace_back(assignedNode->getFlopsPerByte() * sizePerHost);
			activities.emplace_back(assignedNode->getNodeLocalBurstBuffer()->write_async(sizePerHost));
			int index = rank * numNodes + nodeRank++;
			if (assignedNode == node) continue;
			payloads[index] = sizePerHost;
		}
		simgrid::s4u::ActivityPtr parallelActivity = simgrid::s4u::this_actor::exec_init(hosts, flops, payloads);
		parallelActivity->start();
		activities.emplace_back(parallelActivity);
		return activities;
	} else {
		xbt_die("No burst buffer available on node %s", node->getHostName().c_str());
	}
}
