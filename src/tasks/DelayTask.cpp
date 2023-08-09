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
					 std::optional<std::vector<double>> delays, std::optional<std::string> delayModel,
					 VectorPattern delayPattern) :
		Task(name, iterations, synchronized),
		delays(delays.has_value() ? std::move(delays.value()) : std::vector<double>()),
		delayModel(delayModel.has_value() ? std::move(delayModel.value()) : ""),
		delayPattern(delayPattern) {}

void DelayTask::scaleTo(int numNodes, int numGpusPerNode, const std::map<std::string, std::string>& runtimeArguments) {
	Task::scaleTo(numNodes, numGpusPerNode, runtimeArguments);
	delays = Utility::createVector(delayModel, delayPattern, numNodes, numGpusPerNode, runtimeArguments);
}
