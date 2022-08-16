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
#include "Utility.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(BurstBufferWriteTask, "Messages within the burst buffer write task");

BurstBufferWriteTask::BurstBufferWriteTask(const std::string& name, const std::string& iterations, bool synchronized,
										   bool asynchronous, const std::vector<double>& ioSizes,
										   VectorPattern ioPattern) :
		IoTask(name, iterations, synchronized, asynchronous, ioSizes, ioPattern) {}

BurstBufferWriteTask::BurstBufferWriteTask(const std::string& name, const std::string& iterations, bool synchronized,
										   bool asynchronous, const std::string& ioModel, VectorPattern ioPattern) :
		IoTask(name, iterations, synchronized, asynchronous, ioModel, ioPattern) {}

void BurstBufferWriteTask::execute(const Node* node, const Job* job,
								   const std::vector<Node*>& nodes, int rank,
								   simgrid::s4u::BarrierPtr barrier) const {
	if (node->getType() == COMPUTE_NODE_WITH_BB) {
		XBT_INFO("Writing %f bytes to burst buffer", ioSizes[rank]);
		node->getNodeLocalBurstBuffer()->write(ioSizes[rank]);
	} else if (node->getType() == COMPUTE_NODE_WITH_WIDE_STRIPED_BB) {
		std::vector<simgrid::s4u::ActivityPtr> activities;
		int numNodes = nodes.size();
		double sizePerHost = ioSizes[rank] / nodes.size();
		XBT_INFO("Writing %f bytes from burst buffer", sizePerHost);
		for (auto& assignedNode: nodes) {
			if (assignedNode == node) continue;
			XBT_INFO("Writing %f bytes from burst buffer %s", sizePerHost, assignedNode->getHostName().c_str());
		}
		activities.emplace_back(node->getNodeLocalBurstBuffer()->write_async(sizePerHost));
		if (rank == 0) {
			std::vector<simgrid::s4u::Host*> hosts;
			std::vector<Node*> assignedNodes = nodes;
			auto func = [](const Node* node) { return node->getHost(); };
			std::transform(std::begin(assignedNodes), std::end(assignedNodes), std::back_inserter(hosts), func);
			std::vector<double> flops(numNodes, numNodes * node->getFlopsPerByte() * sizePerHost);
			std::vector<double> payloads = Utility::createMatrix(sizePerHost * (numNodes * numNodes - numNodes),
																 ALL_TO_ALL, numNodes);
			simgrid::s4u::this_actor::parallel_execute(hosts, flops, payloads);
		}
		for (auto& activity: activities) {
			activity->wait();
		}
		barrier->wait();
	} else {
		xbt_die("Unknown compute node type");
	}
}
