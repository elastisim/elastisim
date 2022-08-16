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
#include "Task.h"

Phase::Phase(std::deque<std::unique_ptr<Task>> tasks, int iterations, bool schedulingPoint, bool finalSchedulingPoint,
			 bool barrier) : tasks(std::move(tasks)), iterations(iterations),
							 schedulingPoint(schedulingPoint), finalSchedulingPoint(finalSchedulingPoint),
							 barrier(barrier) {
	for (auto& task: Phase::tasks) {
		taskPointers.push_back(task.get());
	}
}

const std::deque<Task*>& Phase::getTasks() const {
	return taskPointers;
}

int Phase::getIterations() const {
	return iterations;
}

void Phase::setIterations(int iterations) {
	Phase::iterations = iterations;
}

bool Phase::hasSchedulingPoint() const {
	return schedulingPoint;
}

bool Phase::hasFinalSchedulingPoint() const {
	return finalSchedulingPoint;
}

bool Phase::hasBarrier() const {
	return barrier;
}

void Phase::scaleTo(int numNodes, int numGpusPerNode) {
	for (auto& task: tasks) {
		task->scaleTo(numNodes, numGpusPerNode);
	}
}
