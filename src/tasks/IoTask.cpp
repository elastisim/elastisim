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
			   std::vector<double> ioSizes, VectorPattern ioPattern) :
		Task(name, iterations, synchronized), asynchronous(asynchronous), ioSizes(std::move(ioSizes)),
		ioPattern(ioPattern) {}

IoTask::IoTask(const std::string& name, const std::string& iterations, bool synchronized, bool asynchronous,
			   std::string ioModel, VectorPattern ioPattern) :
		Task(name, iterations, synchronized), asynchronous(asynchronous), ioModel(std::move(ioModel)),
		ioPattern(ioPattern) {}

bool IoTask::isAsynchronous() const {
	return asynchronous;
}

void IoTask::scaleTo(int numNodes, int numGpusPerNode) {
	Task::scaleTo(numNodes, numGpusPerNode);
	ioSizes = Utility::createVector(ioModel, ioPattern, numNodes, numGpusPerNode);
}
