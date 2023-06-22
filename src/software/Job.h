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
	MALLEABLE = 2
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
	JobType type;
	JobState state;
	double walltime;
	int numNodes;
	int numGpusPerNode;
	int numNodesMin;
	int numNodesMax;
	int numGpusPerNodeMin;
	int numGpusPerNodeMax;
	double submitTime;
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
	int assignedNumGpusPerNode;
	int executingNumGpusPerNode;

public:
	Job(int walltime, int numNodes, int numGpusPerNode, double submitTime,
		std::map<std::string, std::string> arguments, std::map<std::string, std::string> attributes,
		std::unique_ptr<Workload> workload);

	Job(int walltime, JobType type, int numNodesMin, int numNodesMax, int numGpusPerNodeMin, int numGpusPerNodeMax,
		double submitTime, std::map<std::string, std::string> arguments, std::map<std::string, std::string> attributes,
		std::unique_ptr<Workload> workload);

	int getId() const;

	void setId(int id);

	JobState getState() const;

	JobType getType() const;

	void setState(JobState newState);

	double getWalltime() const;

	double getSubmitTime() const;

	double getStartTime() const;

	double getEndTime() const;

	double getWaitTime() const;

	double getMakespan() const;

	double getTurnaroundTime() const;

	const Workload* getWorkload() const;

	const std::vector<Node*>& getExecutingNodes() const;

	const std::vector<Node*>& getExpandingNodes() const;

	void setExpandNodes(std::vector<Node*> expandingNodes);

	void assignNode(Node* node);

	void assignNumGpusPerNode(int numGpusPerNode);

	int getNumberOfExecutingNodes() const;

	int getExecutingNumGpusPerNode() const;

	void advanceWorkload(int completedPhases, int remainingIterations);

	void completeWorkload();

	void updateState();

	void clearAssignedNodes();

	void checkSpecification() const;

	void checkConfigurationValidity() const;

	nlohmann::json toJson();
};


#endif //ELASTISIM_JOB_H
