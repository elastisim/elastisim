/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "Workload.h"

#include <utility>
#include "Phase.h"
#include "Task.h"


Workload::Workload(std::unique_ptr<Phase> initPhase, std::unique_ptr<Phase> reconfigurationPhase,
				   std::unique_ptr<Phase> expansionPhase, std::deque<std::unique_ptr<Phase>> phases)
		: initPhase(std::move(initPhase)), reconfigurationPhase(std::move(reconfigurationPhase)),
		  expansionPhase(std::move(expansionPhase)), phases(std::move(phases)) {
	totalPhaseCount = 0;
	for (const auto& phase: Workload::phases) {
		totalPhaseCount += phase->getIterations();
		phasePointers.push_back(phase.get());
	}
	completedPhases = 0;
}

const Phase* Workload::getInitPhase() const {
	return initPhase.get();
}

const Phase* Workload::getReconfigurationPhase() const {
	return reconfigurationPhase.get();
}

const Phase* Workload::getExpansionPhase() const {
	return expansionPhase.get();
}

const std::deque<const Phase*>& Workload::getPhases() const {
	return phasePointers;
}

int Workload::getTotalPhaseCount() const {
	return totalPhaseCount;
}

int Workload::getCompletedPhases() const {
	return completedPhases;
}

void Workload::scaleInitPhaseTo(int numNodes, int numGpusPerNode, const std::map<std::string, std::string>& runtimeArguments) {
	if (initPhase) {
		initPhase->scaleTo(numNodes, numGpusPerNode, runtimeArguments);
	}
}

void Workload::scaleReconfigurationPhaseTo(int numNodes, int numGpusPerNode, const std::map<std::string, std::string>& runtimeArguments) {
	if (reconfigurationPhase) {
		reconfigurationPhase->scaleTo(numNodes, numGpusPerNode, runtimeArguments);
	}
}

void Workload::scaleExpandPhaseTo(int numNodes, int numGpusPerNode, const std::map<std::string, std::string>& runtimeArguments) {
	if (expansionPhase) {
		expansionPhase->scaleTo(numNodes, numGpusPerNode, runtimeArguments);
	}
}

void Workload::scaleTo(int numNodes, int numGpusPerNode, const std::map<std::string, std::string>& runtimeArguments) {
	for (const auto& phase: phases) {
		phase->scaleTo(numNodes, numGpusPerNode, runtimeArguments);
	}
}

void Workload::advance(int completedPhases, int remainingIterations) {
	for (int i = 0; i < completedPhases; ++i) {
		Workload::completedPhases += phases.front()->getIterations();
		phases.pop_front();
		phasePointers.pop_front();
	}
	if (remainingIterations > 0) {
		Workload::completedPhases += (phases.front()->getIterations() - remainingIterations);
		phases.front()->setIterations(remainingIterations);
	}
}

void Workload::complete() {
	phases.clear();
	completedPhases = totalPhaseCount;
}
