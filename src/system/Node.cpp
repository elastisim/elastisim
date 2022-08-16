/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "Node.h"

#include <simgrid/s4u.hpp>
#include <utility>

#include "Job.h"
#include "Workload.h"
#include "Task.h"
#include "Application.h"
#include "SchedMsg.h"
#include "NodeMsg.h"
#include "AsyncSleep.h"
#include "Configuration.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(ComputeNode, "Messages within the Compute Node actor");

void Node::handleWorkloadCompletion(Job* job) {
	application.erase(job);
	s4u_Mailbox* mailboxScheduler = s4u_Mailbox::by_name("Scheduler");
	mailboxScheduler->put(new SchedMsg(WORKLOAD_PROCESSED, job, this), 0);
}

void Node::handleJobAllocation(Job* job, int rank, const simgrid::s4u::BarrierPtr& jobBarrier) {
	if (!allowOversubscription && !runningJobs.empty()) {
		xbt_die("Node %d already allocated and cannot be assigned to job %d", id, job->getId());
	}
	assignedRank[job] = rank;
	barrier[job] = jobBarrier;
	initializing[job] = true;
	reconfiguring[job] = false;
	expanding[job] = false;
	runningJobs.insert(job);
	if (!runningJobs.empty()) {
		state = NODE_ALLOCATED;
	}
	collectStatistics();
	application[job] = s4u_Actor::create("Application@Job" + std::to_string(job->getId()), host,
										 Application(this, job, assignedRank[job]));
}

void Node::handleSchedulingPoint(Job* job, int completedPhases, int remainingIterations) {
	application.erase(job);
	s4u_Mailbox* mailboxScheduler = s4u_Mailbox::by_name("Scheduler");
	mailboxScheduler->put(new SchedMsg(SCHEDULING_POINT, job, this, completedPhases, remainingIterations),
						  0);
}

void Node::continueJob(Job* job) {
	application[job] = s4u_Actor::create("Application@Job" + std::to_string(job->getId()), host,
										 Application(this, job, assignedRank[job]));
}

void Node::expandJob(Job* job, int rank, int expandRank,
					 const simgrid::s4u::BarrierPtr& jobBarrier,
					 const simgrid::s4u::BarrierPtr& jobExpandBarrier) {
	assignedRank[job] = rank;
	assignedExpandRank[job] = expandRank;
	barrier[job] = jobBarrier;
	expandBarrier[job] = jobExpandBarrier;
	initializing[job] = false;
	reconfiguring[job] = true;
	expanding[job] = true;
	runningJobs.insert(job);
	if (!runningJobs.empty()) {
		state = NODE_ALLOCATED;
	}
	collectStatistics();
	application[job] = s4u_Actor::create("Application@Job" + std::to_string(job->getId()), host,
										 Application(this, job, assignedRank[job]));
}

void Node::completeJob(Job* job) {
	runningJobs.erase(job);
	if (runningJobs.empty()) {
		state = NODE_FREE;
	}
	collectStatistics();
}

void Node::reconfigureJob(Job* job, int rank, const simgrid::s4u::BarrierPtr& jobBarrier) {
	assignedRank[job] = rank;
	barrier[job] = jobBarrier;
	reconfiguring[job] = true;
	application[job] = s4u_Actor::create("Application@Job" + std::to_string(job->getId()), host,
										 Application(this, job, assignedRank[job]));
}

void Node::killJob(Job* job) {
	application[job]->kill();
	application.erase(job);
	runningJobs.erase(job);
	if (runningJobs.empty()) {
		state = NODE_FREE;
	}
	collectStatistics();
}

void Node::collectStatistics() {
	switch (state) {
		case NODE_FREE:
		case NODE_RESERVED:
			nodeUtilizationOutput << simgrid::s4u::Engine::get_clock() << ","
								  << getHostName() << ","
								  << "None" << std::endl;
			break;
		case NODE_ALLOCATED:
			std::string jobIds;
			for (auto& job: runningJobs) {
				jobIds += std::to_string(job->getId()) + ";";
			}
			jobIds.pop_back();
			nodeUtilizationOutput << simgrid::s4u::Engine::get_clock() << ","
								  << getHostName() << ","
								  << jobIds << std::endl;
			break;
	}
}

Node::Node(int id, NodeType type, s4u_Host* host, s4u_Disk* nodeLocalBurstBuffer,
		   std::vector<s4u_Host*> pfsHosts, double flopsPerByte, std::vector<std::unique_ptr<Gpu>> gpus,
		   long gpuToGpuBandwidth, std::ofstream& nodeUtilizationOutput) :
		id(id), type(type), host(host), nodeLocalBurstBuffer(nodeLocalBurstBuffer), state(NODE_FREE),
		pfsHosts(std::move(pfsHosts)), flopsPerByte(flopsPerByte), gpus(std::move(gpus)),
		gpuToGpuBandwidth(gpuToGpuBandwidth), nodeUtilizationOutput(nodeUtilizationOutput),
		allowOversubscription(Configuration::getBoolIfExists("allow_oversubscription")) {
	for (auto& gpu: Node::gpus) {
		gpuPointers.push_back(gpu.get());
	}
}

int Node::getId() const {
	return id;
}

NodeType Node::getType() const {
	return type;
}

s4u_Host* Node::getHost() const {
	return host;
}

std::string Node::getHostName() const {
	return host->get_name();
}

s4u_Disk* Node::getNodeLocalBurstBuffer() const {
	return nodeLocalBurstBuffer;
}

const std::vector<s4u_Host*>& Node::getPfsHosts() const {
	return pfsHosts;
}

double Node::getFlopsPerByte() const {
	return flopsPerByte;
}

const std::vector<Gpu*>& Node::getGpus() const {
	return gpuPointers;
}

long Node::getGpuToGpuBandwidth() const {
	return gpuToGpuBandwidth;
}

void Node::occupyGpuLink() const {
	gpuLinkMutex->lock();
}

void Node::releaseGpuLink() const {
	gpuLinkMutex->unlock();
}

s4u_Mailbox* Node::execGpuTransferAsync(const std::vector<double>& bytes, int numGpus) const {
	s4u_Mailbox* gpuLinkCallback = s4u_Mailbox::by_name("GPULink@" + getHostName());

	int gpuPairs = ((numGpus - 1) * numGpus) / 2;
	std::vector<double> exchangedBytes(gpuPairs);
	int index = 0;
	for (int i = 0; i < numGpus; ++i) {
		for (int j = i + 1; j < numGpus; ++j) {
			exchangedBytes[index] = bytes[i * numGpus + j];
			exchangedBytes[index++] = bytes[j * numGpus + i];
		}
	}
	double maxBytes = *std::max_element(std::begin(exchangedBytes), std::end(exchangedBytes));
	XBT_INFO("Transferring intra-node communication (dominant communication %f bytes) via GPU link", maxBytes);
	s4u_Actor::create("GPULink@" + getHostName(), host,
					  AsyncSleep(maxBytes / gpuToGpuBandwidth,
								 [this]() { this->occupyGpuLink(); },
								 [this]() { this->releaseGpuLink(); },
								 gpuLinkCallback, gpuLinkCallback));

	return gpuLinkCallback;
}

const simgrid::s4u::BarrierPtr& Node::getBarrier(Job* job) const {
	return barrier.at(job);
}

const simgrid::s4u::BarrierPtr& Node::getExpandBarrier(Job* job) const {
	return expandBarrier.at(job);
}

bool Node::isInitializing(Job* job) const {
	return initializing.at(job);
}

void Node::markInitialized(Job* job) {
	initializing[job] = false;
}

bool Node::isReconfiguring(Job* job) const {
	return reconfiguring.at(job);
}

void Node::markReconfigured(Job* job) {
	reconfiguring[job] = false;
}

bool Node::isExpanding(Job* job) const {
	return expanding.at(job);
}

void Node::markExpanded(Job* job) {
	expanding[job] = false;
}

int Node::getExpandRank(Job* job) const {
	return assignedExpandRank.at(job);
}

void Node::expectJob(Job* job) {
	if (!allowOversubscription) {
		if (runningJobs.find(job) == runningJobs.end() && !runningJobs.empty()) {
			xbt_die("Node %d already allocated and cannot be reserved for job %d", id, job->getId());
		}
		if (expectedJobs.find(job) == expectedJobs.end() && !expectedJobs.empty()) {
			xbt_die("Node %d already reserved and cannot be reserved for job %d", id, job->getId());
		}
	}
	expectedJobs.insert(job);
	if (state == NODE_FREE) {
		state = NODE_RESERVED;
	}
}

void Node::removeExpectedJob(Job* job) {
	expectedJobs.erase(job);
	if (expectedJobs.empty() && state == NODE_RESERVED) {
		state = NODE_FREE;
	}
}

void Node::act() {

	// initialization
	collectStatistics();
	s4u_Mailbox* mailbox = s4u_Mailbox::by_name(getHostName());
	gpuLinkMutex = s4u_Mutex::create();

	// main loop
	while (true) {
		auto payload = mailbox->get_unique<NodeMsg>();
		if (payload->getType() == NODE_ALLOCATE) {
			XBT_INFO("Received job to allocate");
			handleJobAllocation(payload->getJob(), payload->getRank(), payload->getBarrier());
		} else if (payload->getType() == NODE_CONTINUE) {
			XBT_INFO("Continuing job %d", payload->getJob()->getId());
			continueJob(payload->getJob());
		} else if (payload->getType() == NODE_RECONFIGURE) {
			XBT_INFO("Reconfiguring job %d", payload->getJob()->getId());
			reconfigureJob(payload->getJob(), payload->getRank(), payload->getBarrier());
		} else if (payload->getType() == NODE_EXPAND) {
			XBT_INFO("Expanding job %d", payload->getJob()->getId());
			expandJob(payload->getJob(), payload->getRank(), payload->getExpandRank(), payload->getBarrier(),
					  payload->getExpandBarrier());
		} else if (payload->getType() == NODE_KILL) {
			XBT_INFO("Killing job %d", payload->getJob()->getId());
			killJob(payload->getJob());
		} else if (payload->getType() == NODE_DEALLOCATE) {
			XBT_INFO("Deallocate job %d", payload->getJob()->getId());
			completeJob(payload->getJob());
		} else if (payload->getType() == AT_SCHEDULING_POINT) {
			handleSchedulingPoint(payload->getJob(), payload->getCompletedPhases(), payload->getRemainingIterations());
		} else if (payload->getType() == WORKLOAD_COMPLETED) {
			handleWorkloadCompletion(payload->getJob());
		} else if (payload->getType() == NODE_FINALIZE) {
			XBT_INFO("Received finalization");
			break;
		}
	}

	// finalization

}

nlohmann::json Node::toJson() {
	nlohmann::json json;
	json["id"] = id;
	json["type"] = type;
	json["state"] = state;
	json["assigned_jobs"] = nlohmann::json::array();
	for (auto& job: runningJobs) {
		json["assigned_jobs"].push_back(job->getId());
	}
	json["gpus"] = nlohmann::json::array();
	for (auto& gpu: gpus) {
		json["gpus"].push_back(gpu->toJson());
	}
	return json;
}
