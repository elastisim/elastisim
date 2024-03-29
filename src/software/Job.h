/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_JOB_H
#define ELASTISIM_JOB_H

#include <memory>
#include <simgrid/s4u.hpp>
#include <list>
#include <json.hpp>

class Node;

class Workload;

enum JobType {
	RIGID = 0,
	MOLDABLE = 1,
	MALLEABLE = 2,
	EVOLVING = 3,
	ADAPTIVE = 4
};

enum JobState {
	PENDING_SUBMISSION = 0,
	PENDING = 1,
	PENDING_ALLOCATION = 2,
	PENDING_KILL = 3,
	RUNNING = 4,
	PENDING_RECONFIGURATION = 5,
	IN_RECONFIGURATION = 6,
	COMPLETED = 7,
	KILLED = 8
};

class Job {

private:
	int id;
	const JobType type;
	JobState state;
	const double walltime;
	const int numNodes;
	const int numGpusPerNode;
	const int numNodesMin;
	const int numNodesMax;
	const int numGpusPerNodeMin;
	const int numGpusPerNodeMax;
	const double submitTime;
	double startTime;
	double endTime;
	double waitTime;
	double makespan;
	double turnaroundTime;
	std::unique_ptr<Workload> workload;
	std::vector<Node*> assignedNodes;
	std::vector<Node*> executingNodes;
	std::vector<Node*> expandingNodes;
	std::map<std::string, std::string> arguments;
	std::map<std::string, std::string> attributes;
	std::map<std::string, std::string> runtimeArguments;
	std::map<std::string, std::string> additionalArguments;
	simgrid::s4u::MutexPtr runtimeArgumentsMutex;
	int assignedNumGpusPerNode;
	int executingNumGpusPerNode;
	const bool clipEvolvingRequests;

public:
	Job(int walltime, int numNodes, int numGpusPerNode, double submitTime,
		std::map<std::string, std::string> arguments, std::map<std::string, std::string> attributes,
		std::unique_ptr<Workload> workload);

	Job(int walltime, JobType type, int numNodesMin, int numNodesMax, int numGpusPerNodeMin, int numGpusPerNodeMax,
		double submitTime, std::map<std::string, std::string> arguments, std::map<std::string, std::string> attributes,
		std::unique_ptr<Workload> workload);

	[[nodiscard]] int getId() const;

	void setId(int id);

	[[nodiscard]] JobState getState() const;

	[[nodiscard]] JobType getType() const;

	void setState(JobState newState);

	[[nodiscard]] double getWalltime() const;

	[[nodiscard]] double getSubmitTime() const;

	[[nodiscard]] double getStartTime() const;

	[[nodiscard]] double getEndTime() const;

	[[nodiscard]] double getWaitTime() const;

	[[nodiscard]] double getMakespan() const;

	[[nodiscard]] double getTurnaroundTime() const;

	[[nodiscard]] const Workload* getWorkload() const;

	[[nodiscard]] const std::vector<Node*>& getExecutingNodes() const;

	[[nodiscard]] const std::vector<Node*>& getExpandingNodes() const;

	void setExpandNodes(std::vector<Node*> expandingNodes);

	[[nodiscard]] int calculateEvolvingRequest(const std::string& evolvingModel, int phaseIteration);

	void assignNode(Node* node);

	void assignNumGpusPerNode(int numGpusPerNode);

	[[nodiscard]] int getNumberOfExecutingNodes() const;

	[[nodiscard]] int getExecutingNumGpusPerNode() const;

	void advanceWorkload(int completedPhases, int remainingIterations);

	void completeWorkload();

	void updateState();

	void clearAssignedNodes();

	void updateRuntimeArguments(const std::string& key, const std::string& value);

	void clearRuntimeArguments();

	void checkSpecification() const;

	void checkConfigurationValidity() const;

	[[nodiscard]] nlohmann::json toJson() const;

};


#endif //ELASTISIM_JOB_H
