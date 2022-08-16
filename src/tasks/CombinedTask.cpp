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
#include "Job.h"
#include "Utility.h"

CombinedTask::CombinedTask(const std::string& name, const std::string& iterations, bool synchronized,
						   std::vector<double> flops) :
		Task(name, iterations, synchronized), flops(std::move(flops)) {}

CombinedTask::CombinedTask(const std::string& name, const std::string& iterations, bool synchronized,
						   std::vector<double> flops, std::string communicationModel,
						   MatrixPattern communicationPattern) :
		Task(name, iterations, synchronized), flops(std::move(flops)),
		communicationModel(std::move(communicationModel)),
		communicationPattern(communicationPattern) {}

CombinedTask::CombinedTask(const std::string& name, const std::string& iterations, bool synchronized,
						   std::string computationModel, VectorPattern computationPattern) :
		Task(name, iterations, synchronized), computationModel(std::move(computationModel)),
		computationPattern(computationPattern) {}

CombinedTask::CombinedTask(const std::string& name, const std::string& iterations, bool synchronized,
						   std::string computationModel, VectorPattern computationPattern,
						   std::string communicationModel, MatrixPattern communicationPattern) :
		Task(name, iterations, synchronized), computationModel(std::move(computationModel)),
		computationPattern(computationPattern), communicationModel(std::move(communicationModel)),
		communicationPattern(communicationPattern) {}

void CombinedTask::scaleTo(int numNodes, int numGpusPerNode) {
	Task::scaleTo(numNodes, numGpusPerNode);
	if (!computationModel.empty()) {
		flops = Utility::createVector(computationModel, computationPattern, numNodes, numGpusPerNode);
	}
}
