/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_NODE_H
#define ELASTISIM_NODE_H

#include <vector>
#include <simgrid/s4u.hpp>
#include <fstream>
#include <json.hpp>
#include <stack>
#include "Gpu.h"

class Task;

class Phase;

class Job;

enum NodeType {
	COMPUTE_NODE = 0,
	COMPUTE_NODE_WITH_BB = 1,
	COMPUTE_NODE_WITH_WIDE_STRIPED_BB = 2,
};

enum NodeState {
	NODE_FREE = 0,
	NODE_ALLOCATED = 1,
	NODE_RESERVED = 2
};

class Node {

private:
	const int id;
	const NodeType type;
	s4u_Host* host;
	s4u_Disk* nodeLocalBurstBuffer;
	std::vector<s4u_Host*> pfsHosts;
	NodeState state;
	std::set<Job*> runningJobs;
	std::unordered_map<Job*, int> assignedRank;
	std::unordered_map<Job*, int> assignedExpandRank;
	std::unordered_map<Job*, s4u_Actor*> application;
	std::unordered_map<Job*, simgrid::s4u::BarrierPtr> barrier;
	std::unordered_map<Job*, simgrid::s4u::BarrierPtr> expandBarrier;
	std::ofstream& nodeUtilizationOutput;
	std::unordered_map<Job*, bool> initializing;
	std::unordered_map<Job*, bool> reconfiguring;
	std::unordered_map<Job*, bool> expanding;
	const double flopsPerByte;
	std::vector<std::unique_ptr<Gpu>> gpus;
	std::vector<const Gpu*> gpuPointers;
	const long gpuToGpuBandwidth;
	simgrid::s4u::MutexPtr gpuLinkMutex;
	std::set<Job*> expectedJobs;
	const bool allowOversubscription;
	const bool logTaskTimes;
	std::ofstream& taskTimes;

	void collectStatistics();

public:
	Node(int id, NodeType type, s4u_Host* host, s4u_Disk* nodeLocalBurstBuffer, std::vector<s4u_Host*> pfsHosts,
		 double flopsPerByte, std::vector<std::unique_ptr<Gpu>> gpus, long gpuToGpuBandwidth,
		 std::ofstream& nodeUtilizationOutput, std::ofstream& taskTimes);

	void allocateJob(Job* job, int rank, const simgrid::s4u::BarrierPtr& jobBarrier);

	void continueJob(Job* job);

	void reconfigureJob(Job* job, int rank, const simgrid::s4u::BarrierPtr& jobBarrier);

	void expandJob(Job* job, int rank, int expandRank, const simgrid::s4u::BarrierPtr& jobBarrier,
				   const simgrid::s4u::BarrierPtr& jobExpandBarrier);

	void completeJob(Job* job);

	void killJob(Job* job);

	[[nodiscard]] int getId() const;

	[[nodiscard]] NodeType getType() const;

	[[nodiscard]] s4u_Host* getHost() const;

	[[nodiscard]] std::string getHostName() const;

	[[nodiscard]] s4u_Disk* getNodeLocalBurstBuffer() const;

	[[nodiscard]] const std::vector<s4u_Host*>& getPfsHosts() const;

	[[nodiscard]] double getFlopsPerByte() const;

	[[nodiscard]] const std::vector<const Gpu*>& getGpus() const;

	[[nodiscard]] long getGpuToGpuBandwidth() const;

	[[nodiscard]] std::vector<s4u_Mailbox*> execGpuComputationAsync(int numGpus, double flopsPerGpu) const;

	void occupyGpuLink() const;

	void releaseGpuLink() const;

	[[nodiscard]] s4u_Mailbox* execGpuTransferAsync(const std::vector<double>& bytes, int numGpus) const;

	[[nodiscard]] const simgrid::s4u::BarrierPtr& getBarrier(Job* job) const;

	[[nodiscard]] const simgrid::s4u::BarrierPtr& getExpandBarrier(Job* job) const;

	[[nodiscard]] bool isInitializing(Job* job) const;

	void markInitialized(Job* job);

	[[nodiscard]] bool isReconfiguring(Job* job) const;

	void markReconfigured(Job* job);

	[[nodiscard]] bool isExpanding(Job* job) const;

	void markExpanded(Job* job);

	[[nodiscard]] int getExpandRank(Job* job) const;

	void expectJob(Job* job);

	void removeExpectedJob(Job* job);

	void logTaskTime(const Job* job, const Task* task, double duration) const;

	[[nodiscard]] nlohmann::json toJson();

};


#endif //ELASTISIM_NODE_H
