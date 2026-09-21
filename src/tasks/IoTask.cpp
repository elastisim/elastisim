/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "IoTask.h"

#include <utility>
#include "Utility.h"

IoTask::IoTask(const std::string& name, const std::string& iterations, bool synchronized, bool asynchronous,
			   std::optional<std::vector<double>> ioSizes, std::optional<std::string> ioModel,
			   std::optional<std::vector<double>> bandwidths, std::string bandwidthModel,
			   VectorPattern ioPattern) :
		Task(name, iterations, synchronized), asynchronous(asynchronous),
		ioSizes(ioSizes.has_value() ? std::move(ioSizes.value()) : std::vector<double>()),
		ioModel(ioModel.has_value() ? std::move(ioModel.value()) : ""), ioPattern(ioPattern),
		bandwidths(bandwidths.has_value() ? std::move(bandwidths.value()) : std::vector<double>()),
		bandwidthModel(std::move(bandwidthModel)) {}

bool IoTask::isAsynchronous() const {
	return asynchronous;
}

void IoTask::scaleTo(int numNodes, int numGpusPerNode, const std::map<std::string, std::string>& runtimeArguments) {
	Task::scaleTo(numNodes, numGpusPerNode, runtimeArguments);
	ioSizes = Utility::createVector(ioModel, ioPattern, numNodes, numGpusPerNode, runtimeArguments);
	bandwidths = Utility::createBandwidthVector(bandwidthModel, ioPattern, numNodes, numGpusPerNode);
}
