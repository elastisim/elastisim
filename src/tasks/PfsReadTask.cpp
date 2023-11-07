/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "PfsReadTask.h"

#include <simgrid/s4u.hpp>
#include "Node.h"
#include "Job.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(PfsReadTask, "Messages within the PFS read task");

PfsReadTask::PfsReadTask(const std::string& name, const std::string& iterations, bool synchronized, bool asynchronous,
						 const std::optional<std::vector<double>>& ioSizes, const std::optional<std::string>& ioModel,
						 VectorPattern ioPattern) :
		IoTask(name, iterations, synchronized, asynchronous, ioSizes, ioModel, ioPattern) {}

void PfsReadTask::execute(const Node* node, const Job* job, const std::vector<Node*>& nodes, int rank,
						  simgrid::s4u::BarrierPtr barrier) const {
	std::vector<simgrid::s4u::ActivityPtr> activities = executeAsync(node, job, nodes, rank);
	for (const auto& activity: activities) {
		activity->wait();
	}
}

std::vector<simgrid::s4u::ActivityPtr>
PfsReadTask::executeAsync(const Node* node, const Job* job,
						  const std::vector<Node*>& nodes, int rank) const {
	if (ioSizes[rank] > 0) {
		XBT_INFO("Reading %f bytes from PFS", ioSizes[rank]);
	}
	std::vector<s4u_Host*> hosts = {node->getHost()};
	std::vector<s4u_Host*> pfsHosts = node->getPfsHosts();
	hosts.insert(std::end(hosts), std::begin(pfsHosts), std::end(pfsHosts));
	size_t numHosts = hosts.size();
	std::vector<double> empty(numHosts);
	std::vector<double> payloads(numHosts * numHosts, 0);
	double payloadPerHost = ioSizes[rank] / (numHosts - 1);
	for (int i = numHosts; i < numHosts * numHosts; i += numHosts) {
		payloads[i] = payloadPerHost;
	}
	simgrid::s4u::ActivityPtr activity = simgrid::s4u::this_actor::exec_init(hosts, empty, payloads);
	activity->start();
	return {activity};
}

