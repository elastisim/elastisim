/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "Task.h"

#include <utility>
#include "Utility.h"

Task::Task(std::string name, std::string iterations, bool synchronized) :
		name(std::move(name)), iterationModel(std::move(iterations)), synchronized(synchronized) {}

Task::~Task() = default;

const std::string& Task::getName() const {
	return name;
}

int Task::getIterations() const {
	return iterations;
}

void Task::updateIterations(int numNodes, int numGpusPerNode) {
	iterations = floor(Utility::evaluateFormula(iterationModel, numNodes, numGpusPerNode));
}

bool Task::isSynchronized() const {
	return synchronized;
}

bool Task::isAsynchronous() const {
	return false;
}

std::vector<simgrid::s4u::ActivityPtr>
Task::executeAsync(const Node* node, const Job* job, const std::vector<Node*>& nodes, int rank) const {
	xbt_die("Task does not support asynchronous execution");
}

void Task::scaleTo(int numNodes, int numGpusPerNode) {
	iterations = (int) Utility::evaluateFormula(iterationModel, numNodes, numGpusPerNode);
}
