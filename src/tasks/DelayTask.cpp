/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "DelayTask.h"

#include <utility>
#include "Utility.h"

DelayTask::DelayTask(const std::string& name, const std::string& iterations, bool synchronized,
					 std::vector<double> delays, VectorPattern delayPattern) :
		Task(name, iterations, synchronized), delays(std::move(delays)), delayPattern(delayPattern) {}

DelayTask::DelayTask(const std::string& name, const std::string& iterations, bool synchronized, std::string delayModel,
					 VectorPattern delayPattern) :
		Task(name, iterations, synchronized), delayModel(std::move(delayModel)), delayPattern(delayPattern) {}

void DelayTask::scaleTo(int numNodes, int numGpusPerNode) {
	Task::scaleTo(numNodes, numGpusPerNode);
	delays = Utility::createVector(delayModel, delayPattern, numNodes, numGpusPerNode);
}
