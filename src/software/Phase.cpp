/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "Phase.h"

#include <utility>
#include "Task.h"

Phase::Phase(std::deque<std::unique_ptr<Task>> tasks, int iterations, bool schedulingPoint, std::string evolvingModel,
			 bool barrier) :
		tasks(std::move(tasks)), iterations(iterations), initialIterations(iterations),
		schedulingPoint(schedulingPoint), evolvingModel(std::move(evolvingModel)), barrier(barrier) {
	for (const auto& task: Phase::tasks) {
		taskPointers.push_back(task.get());
	}
}

const std::deque<const Task*>& Phase::getTasks() const {
	return taskPointers;
}

int Phase::getIterations() const {
	return iterations;
}

void Phase::setIterations(int iterations) {
	Phase::iterations = iterations;
}

int Phase::getInitialIterations() const {
	return initialIterations;
}

bool Phase::hasSchedulingPoint() const {
	return schedulingPoint;
}

const std::string& Phase::getEvolvingModel() const {
	return evolvingModel;
}

bool Phase::hasEvolvingRequest() const {
	return !evolvingModel.empty();
}

bool Phase::hasBarrier() const {
	return barrier;
}

void Phase::scaleTo(int numNodes, int numGpusPerNode, const std::map<std::string, std::string>& runtimeArguments) {
	for (const auto& task: tasks) {
		task->scaleTo(numNodes, numGpusPerNode, runtimeArguments);
	}
}
