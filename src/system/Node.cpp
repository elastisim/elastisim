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
#include "AsyncSleep.h"
#include "Configuration.h"
#include "PlatformManager.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(ComputeNode, "Messages within the Compute Node actor");

Node::Node(int id, NodeType type, s4u_Host* host, s4u_Disk* nodeLocalBurstBuffer,
		   std::vector<s4u_Host*> pfsHosts, double flopsPerByte, std::vector<std::unique_ptr<Gpu>> gpus,
		   long gpuToGpuBandwidth, std::ofstream& nodeUtilizationOutput, std::ofstream& taskTimes) :
		id(id), type(type), host(host), nodeLocalBurstBuffer(nodeLocalBurstBuffer), pfsHosts(std::move(pfsHosts)),
		state(NODE_FREE), nodeUtilizationOutput(nodeUtilizationOutput), flopsPerByte(flopsPerByte),
		gpus(std::move(gpus)), gpuToGpuBandwidth(gpuToGpuBandwidth), gpuLinkMutex(s4u_Mutex::create()),
		allowOversubscription(Configuration::getBoolIfExists("allow_oversubscription")),
		logTaskTimes(taskTimes.is_open()), taskTimes(taskTimes) {
	for (const auto& gpu: Node::gpus) {
		gpuPointers.push_back(gpu.get());
	}
	collectStatistics();
	PlatformManager::addModifiedComputeNode(this);
}

void Node::collectStatistics() {
	std::string stateStr;
	switch (state) {
		case NODE_FREE:
			stateStr = "free";
			break;
		case NODE_ALLOCATED:
			stateStr = "allocated";
			break;
		case NODE_RESERVED:
			stateStr = "reserved";
			break;
	}
	std::string expectedJobIds = "none";
	if (!expectedJobs.empty()) {
		expectedJobIds.clear();
		for (const auto& job: expectedJobs) {
			expectedJobIds += std::to_string(job->getId()) + ";";
		}
		expectedJobIds.pop_back();
	}
	std::string jobIds = "none";
	if (!runningJobs.empty()) {
		jobIds.clear();
		for (const auto& job: runningJobs) {
			jobIds += std::to_string(job->getId()) + ";";
		}
		jobIds.pop_back();
	}
	nodeUtilizationOutput << simgrid::s4u::Engine::get_clock() << ","
						  << getHostName() << ","
						  << stateStr << ","
						  << jobIds << ","
						  << expectedJobIds << std::endl;
}

void Node::allocateJob(Job* job, int rank, const simgrid::s4u::BarrierPtr& jobBarrier) {
	if (!allowOversubscription && !runningJobs.empty()) {
		xbt_die("Node %d already allocated to job %d and cannot be assigned to job %d", id,
				(*runningJobs.begin())->getId(), job->getId());
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
	PlatformManager::addModifiedComputeNode(this);
	collectStatistics();
	application[job] = s4u_Actor::create("Application@Job" + std::to_string(job->getId()), host,
										 Application(this, job, assignedRank[job], logTaskTimes));
}

void Node::continueJob(Job* job) {
	application[job] = s4u_Actor::create("Application@Job" + std::to_string(job->getId()), host,
										 Application(this, job, assignedRank[job], logTaskTimes));
}

void Node::reconfigureJob(Job* job, int rank, const simgrid::s4u::BarrierPtr& jobBarrier) {
	assignedRank[job] = rank;
	barrier[job] = jobBarrier;
	reconfiguring[job] = true;
	application[job] = s4u_Actor::create("Application@Job" + std::to_string(job->getId()), host,
										 Application(this, job, assignedRank[job], logTaskTimes));
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
	PlatformManager::addModifiedComputeNode(this);
	collectStatistics();
	application[job] = s4u_Actor::create("Application@Job" + std::to_string(job->getId()), host,
										 Application(this, job, assignedRank[job], logTaskTimes));
}

void Node::completeJob(Job* job) {
	application.erase(job);
	runningJobs.erase(job);
	if (runningJobs.empty()) {
		state = NODE_FREE;
	}
	PlatformManager::addModifiedComputeNode(this);
	collectStatistics();
}

void Node::killJob(Job* job) {
	application[job]->kill();
	application.erase(job);
	runningJobs.erase(job);
	if (runningJobs.empty()) {
		state = NODE_FREE;
	}
	PlatformManager::addModifiedComputeNode(this);
	collectStatistics();
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

const std::vector<const Gpu*>& Node::getGpus() const {
	return gpuPointers;
}

long Node::getGpuToGpuBandwidth() const {
	return gpuToGpuBandwidth;
}

std::vector<s4u_Mailbox*> Node::execGpuComputationAsync(int numGpus, double flopsPerGpu) const {
	if (numGpus == 1) {
		XBT_INFO("Processing %f FLOPS on one GPU", flopsPerGpu);
	} else {
		XBT_INFO("Processing %f FLOPS on %u GPUs (%f each)", numGpus * flopsPerGpu, numGpus, flopsPerGpu);
	}
	std::vector<Gpu*> gpuCandidates;
	std::vector<Gpu*> allocatedGpus;
	for (const auto& gpu: gpus) {
		if (gpu->getState() == GPU_FREE) {
			gpuCandidates.push_back(gpu.get());
		} else {
			allocatedGpus.push_back(gpu.get());
		}
	}
	gpuCandidates.insert(std::end(gpuCandidates), std::begin(allocatedGpus), std::end(allocatedGpus));
	std::vector<s4u_Mailbox*> gpuCallbacks;
	gpuCallbacks.reserve(numGpus);
	for (int i = 0; i < numGpus; ++i) {
		gpuCallbacks.push_back(gpuCandidates[i]->execAsync(flopsPerGpu));
	}
	return gpuCallbacks;
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
	double maxBytes = 0;
	for (int i = 0; i < numGpus; ++i) {
		for (int j = i + 1; j < numGpus; ++j) {
			maxBytes = std::max(maxBytes, bytes[i * numGpus + j] + bytes[j * numGpus + i]);
		}
	}
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
	PlatformManager::addModifiedComputeNode(this);
}

void Node::removeExpectedJob(Job* job) {
	expectedJobs.erase(job);
	if (expectedJobs.empty() && state == NODE_RESERVED) {
		state = NODE_FREE;
	}
	PlatformManager::addModifiedComputeNode(this);
}

void Node::logTaskTime(const Job* job, const Task* task, double duration) const {
	taskTimes << simgrid::s4u::Engine::get_clock() << "," << job->getId() << "," << getHostName() << ","
			  << task->getName() << "," << duration << std::endl;
}

nlohmann::json Node::toJson() {
	nlohmann::json json;
	json["id"] = id;
	json["type"] = type;
	json["state"] = state;
	json["assigned_jobs"] = nlohmann::json::array();
	for (const auto& job: runningJobs) {
		json["assigned_jobs"].push_back(job->getId());
	}
	json["gpus"] = nlohmann::json::array();
	for (const auto& gpu: gpus) {
		json["gpus"].push_back(gpu->toJson());
	}
	return json;
}
