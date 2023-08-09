/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "CombinedTask.h"
#include "Node.h"

#include <simgrid/s4u.hpp>
#include <utility>
#include "Utility.h"

CombinedTask::CombinedTask(const std::string& name, const std::string& iterations, bool synchronized,
						   std::optional<std::vector<double>> flops, std::optional<std::string> computationModel,
						   VectorPattern computationPattern, std::optional<std::string> communicationModel,
						   MatrixPattern communicationPattern) :
		Task(name, iterations, synchronized),
		flops(flops.has_value() ? std::move(flops.value()) : std::vector<double>()),
		computationModel(computationModel.has_value() ? std::move(computationModel.value()) : ""),
		computationPattern(computationPattern),
		communicationModel(communicationModel.has_value() ? std::move(communicationModel.value()) : ""),
		communicationPattern(communicationPattern) {}

void
CombinedTask::scaleTo(int numNodes, int numGpusPerNode, const std::map<std::string, std::string>& runtimeArguments) {
	Task::scaleTo(numNodes, numGpusPerNode, runtimeArguments);
	if (!computationModel.empty()) {
		flops = Utility::createVector(computationModel, computationPattern, numNodes, numGpusPerNode, runtimeArguments);
	}
}
