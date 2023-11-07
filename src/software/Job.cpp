/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */


#include "Job.h"

#include <utility>
#include "Workload.h"
#include "Phase.h"
#include "Node.h"
#include "Task.h"
#include "Utility.h"
#include "Configuration.h"

Job::Job(int walltime, int numNodes, int numGpusPerNode, double submitTime,
		 std::map<std::string, std::string> arguments, std::map<std::string, std::string> attributes,
		 std::unique_ptr<Workload> workload) :
		id(-1), type(RIGID), state(PENDING_SUBMISSION), walltime(walltime), numNodes(numNodes),
		numGpusPerNode(numGpusPerNode), numNodesMin(-1), numNodesMax(-1), numGpusPerNodeMin(-1), numGpusPerNodeMax(-1),
		submitTime(submitTime), startTime(-1), endTime(-1), waitTime(-1), makespan(-1), turnaroundTime(-1),
		workload(std::move(workload)), arguments(std::move(arguments)), attributes(std::move(attributes)),
		runtimeArgumentsMutex(s4u_Mutex::create()), assignedNumGpusPerNode(0), executingNumGpusPerNode(0),
		clipEvolvingRequests(false) {
	checkSpecification();
}

Job::Job(int walltime, JobType type, int numNodesMin, int numNodesMax, int numGpusPerNodeMin, int numGpusPerNodeMax,
		 double submitTime, std::map<std::string, std::string> arguments, std::map<std::string, std::string> attributes,
		 std::unique_ptr<Workload> workload) :
		id(-1), type(type), state(PENDING_SUBMISSION), walltime(walltime),
		numNodes(-1), numGpusPerNode(-1), numNodesMin(numNodesMin), numNodesMax(numNodesMax),
		numGpusPerNodeMin(numGpusPerNodeMin), numGpusPerNodeMax(numGpusPerNodeMax), submitTime(submitTime),
		startTime(-1), endTime(-1), waitTime(-1), makespan(-1), turnaroundTime(-1), workload(std::move(workload)),
		arguments(std::move(arguments)), attributes(std::move(attributes)), runtimeArgumentsMutex(s4u_Mutex::create()),
		assignedNumGpusPerNode(0), executingNumGpusPerNode(0),
		clipEvolvingRequests(!Configuration::exists("clip_evolving_requests") ||
							 (bool) Configuration::get("clip_evolving_requests")) {
	checkSpecification();
	additionalArguments["num_nodes_min"] = std::to_string(numNodesMin);
	additionalArguments["num_nodes_max"] = std::to_string(numNodesMax);
}

int Job::getId() const {
	return id;
}

void Job::setId(int id) {
	Job::id = id;
}

JobType Job::getType() const {
	return type;
}

JobState Job::getState() const {
	return state;
}

void Job::setState(JobState newState) {
	if (state == PENDING_ALLOCATION) {
		if (newState == RUNNING) {
			startTime = simgrid::s4u::Engine::get_clock();
			waitTime = startTime - submitTime;
			executingNodes = assignedNodes;
			if (type == RIGID) {
				executingNumGpusPerNode = numGpusPerNode;
			} else {
				size_t numNodes = executingNodes.size();
				executingNumGpusPerNode = assignedNumGpusPerNode;
				workload->scaleTo(numNodes, executingNumGpusPerNode, runtimeArguments);
				workload->scaleInitPhaseTo(numNodes, executingNumGpusPerNode, runtimeArguments);
			}
		}
	} else if (state == PENDING_RECONFIGURATION) {
		if (newState == IN_RECONFIGURATION) {
			executingNodes = assignedNodes;
			for (const auto& node: assignedNodes) {
				node->removeExpectedJob(this);
			}
			executingNumGpusPerNode = assignedNumGpusPerNode;
			size_t numNodes = executingNodes.size();
			workload->scaleTo(numNodes, executingNumGpusPerNode, runtimeArguments);
			workload->scaleReconfigurationPhaseTo(numNodes, executingNumGpusPerNode, runtimeArguments);
		}
	}
	if (newState == COMPLETED || newState == KILLED) {
		endTime = simgrid::s4u::Engine::get_clock();
		makespan = endTime - startTime;
		turnaroundTime = endTime - submitTime;
		for (const auto& node: assignedNodes) {
			node->removeExpectedJob(this);
		}
	}
	state = newState;
}

double Job::getWalltime() const {
	return walltime;
}

double Job::getSubmitTime() const {
	return submitTime;
}

double Job::getStartTime() const {
	return startTime;
}


double Job::getEndTime() const {
	return endTime;
}

double Job::getWaitTime() const {
	return waitTime;
}

double Job::getMakespan() const {
	return makespan;
}

double Job::getTurnaroundTime() const {
	return turnaroundTime;
}

const Workload* Job::getWorkload() const {
	return workload.get();
}

const std::vector<Node*>& Job::getExecutingNodes() const {
	return executingNodes;
}

const std::vector<Node*>& Job::getExpandingNodes() const {
	return expandingNodes;
}

void Job::setExpandNodes(const std::vector<Node*> expandingNodes) {
	Job::expandingNodes = expandingNodes;
	workload->scaleExpandPhaseTo(expandingNodes.size(), executingNumGpusPerNode, runtimeArguments);
}

int Job::calculateEvolvingRequest(const std::string& evolvingModel, int phaseIteration) {
	additionalArguments["phase_iteration"] = std::to_string(phaseIteration);
	int numberOfNodes = (int) Utility::evaluateFormula(evolvingModel, getNumberOfExecutingNodes(),
													   executingNumGpusPerNode, runtimeArguments, additionalArguments);
	if (clipEvolvingRequests) {
		numberOfNodes = std::max(std::min(numberOfNodes, numNodesMax), numNodesMin);
	} else {
		if (numberOfNodes < numNodesMin) {
			xbt_die("Evolving requests can not be smaller than the minimum number of requested nodes "
					"(request model ⌊%s⌋ results in %d, minimum number of nodes is %d)",
					evolvingModel.c_str(), numberOfNodes, numNodesMin);
		}
		if (numberOfNodes > numNodesMax) {
			xbt_die("Evolving requests can not be greater than the maximum number of requested nodes "
					"(request model ⌊%s⌋ results in %d, maximum number of nodes is %d)",
					evolvingModel.c_str(), numberOfNodes, numNodesMax);
		}
	}

	return numberOfNodes;
}

void Job::assignNode(Node* node) {
	if (state == PENDING) {
		assignedNodes.push_back(node);
	} else if (type == MALLEABLE || type == EVOLVING || type == ADAPTIVE) {
		assignedNodes.push_back(node);
		node->expectJob(this);
	} else {
		xbt_die("Assigning nodes during runtime not allowed for rigid/moldable job %d", id);
	}
}

void Job::assignNumGpusPerNode(int numGpusPerNode) {
	Job::assignedNumGpusPerNode = numGpusPerNode;
}

int Job::getNumberOfExecutingNodes() const {
	return executingNodes.size();
}

int Job::getExecutingNumGpusPerNode() const {
	return executingNumGpusPerNode;
}

void Job::advanceWorkload(int completedPhases, int remainingIterations) {
	workload->advance(completedPhases, remainingIterations);
}

void Job::completeWorkload() {
	workload->complete();
}

void Job::updateState() {
	if (assignedNodes != executingNodes) {
		if (state == PENDING) {
			state = PENDING_ALLOCATION;
		} else if (state == RUNNING) {
			state = PENDING_RECONFIGURATION;
		}
	} else {
		if (state == PENDING_RECONFIGURATION) {
			state = RUNNING;
		}
	}
}

void Job::clearAssignedNodes() {
	for (const auto& node: assignedNodes) {
		node->removeExpectedJob(this);
	}
	assignedNodes.clear();
}

void Job::updateRuntimeArguments(const std::string& key, const std::string& value) {
	runtimeArgumentsMutex->lock();
	runtimeArguments[key] = value;
	runtimeArgumentsMutex->unlock();
}

void Job::clearRuntimeArguments() {
	runtimeArgumentsMutex->lock();
	runtimeArguments.clear();
	runtimeArgumentsMutex->unlock();
}

void Job::checkSpecification() const {
	if (type != RIGID) {
		if (numNodesMin < 1) {
			xbt_die("Invalid specification for non-rigid job: number of minimum nodes cannot be less than 1");
		}
		if (numNodesMax < 1) {
			xbt_die("Invalid specification for non-rigid job: number of maximum nodes cannot be less than 1");
		}
		if (numNodesMin > numNodesMax) {
			xbt_die("Invalid specification for non-rigid job: minimum number of nodes (%d) is greater than the maximum number of nodes (%d).",
					numNodesMin, numNodesMax);
		}
		if (numGpusPerNodeMin > numGpusPerNodeMax) {
			xbt_die("Invalid specification for non-rigid job: minimum number of GPUs per node (%d) is greater than the maximum number of GPUs per node (%d).",
					numGpusPerNodeMin, numGpusPerNodeMax);
		}
	} else {
		if (numNodes < 1) {
			xbt_die("Invalid specification for rigid job: number of nodes cannot be less than 1");
		}
	}
}

void Job::checkConfigurationValidity() const {
	size_t numAssignedNodes = assignedNodes.size();
	if (type != RIGID) {
		if (numAssignedNodes < numNodesMin || numAssignedNodes > numNodesMax) {
			xbt_die("Invalid configuration for job %d: Number of assigned nodes is expected to be [%d-%d] but is %zu",
					id, numNodesMin, numNodesMax, numAssignedNodes);
		}
		if (assignedNumGpusPerNode < numGpusPerNodeMin || assignedNumGpusPerNode > numGpusPerNodeMax) {
			xbt_die("Invalid configuration for job %d: Number of assigned GPUs per node is expected to be [%d-%d] but is %u",
					id, numGpusPerNodeMin, numGpusPerNodeMax, assignedNumGpusPerNode);
		}
	} else {
		if (numAssignedNodes != numNodes) {
			xbt_die("Invalid configuration for job %d: Number of assigned nodes is expected to be %d but is %zu",
					id, numNodes, numAssignedNodes);
		}
	}
}

nlohmann::json Job::toJson() const {
	nlohmann::json json;
	json["id"] = id;
	json["state"] = state;
	json["type"] = type;
	json["walltime"] = walltime;
	if (type != RIGID) {
		json["num_nodes_min"] = numNodesMin;
		json["num_nodes_max"] = numNodesMax;
		json["num_gpus_per_node_min"] = numGpusPerNodeMin;
		json["num_gpus_per_node_max"] = numGpusPerNodeMax;
	} else {
		json["num_nodes"] = numNodes;
		json["num_gpus_per_node"] = numGpusPerNode;
	}
	json["submit_time"] = submitTime;
	json["start_time"] = startTime;
	json["end_time"] = endTime;
	json["wait_time"] = waitTime;
	json["makespan"] = makespan;
	json["turnaround_time"] = turnaroundTime;
	json["assigned_nodes"] = nlohmann::json::array();
	for (const auto& node: assignedNodes) {
		json["assigned_nodes"].push_back(node->getId());
	}
	json["assigned_num_gpus_per_node"] = assignedNumGpusPerNode;
	for (const auto& [key, value]: arguments) {
		json["arguments"][key] = value;
	}
	for (const auto& [key, value]: attributes) {
		json["attributes"][key] = value;
	}
	for (const auto& [key, value]: runtimeArguments) {
		json["runtime_arguments"][key] = value;
	}
	json["total_phase_count"] = workload->getTotalPhaseCount();
	json["completed_phases"] = workload->getCompletedPhases();
	return json;
}
