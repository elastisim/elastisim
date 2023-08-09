/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_WORKLOAD_H
#define ELASTISIM_WORKLOAD_H

#include <memory>
#include <queue>
#include <map>

class Task;

class Phase;

class Workload {

private:
	std::unique_ptr<Phase> initPhase;
	std::unique_ptr<Phase> reconfigurationPhase;
	std::unique_ptr<Phase> expansionPhase;
	std::deque<std::unique_ptr<Phase>> phases;
	std::deque<const Phase*> phasePointers;
	int totalPhaseCount;
	int completedPhases;

public:
	Workload(std::unique_ptr<Phase> initPhase, std::unique_ptr<Phase> reconfigurationPhase,
			 std::unique_ptr<Phase> expansionPhase, std::deque<std::unique_ptr<Phase>> phases);

	[[nodiscard]] const Phase* getInitPhase() const;

	[[nodiscard]] const Phase* getReconfigurationPhase() const;

	[[nodiscard]] const Phase* getExpansionPhase() const;

	[[nodiscard]] const std::deque<const Phase*>& getPhases() const;

	[[nodiscard]] int getTotalPhaseCount() const;

	[[nodiscard]] int getCompletedPhases() const;

	void scaleInitPhaseTo(int numNodes, int numGpusPerNode, const std::map<std::string, std::string>& runtimeArguments);

	void scaleReconfigurationPhaseTo(int numNodes, int numGpusPerNode, const std::map<std::string, std::string>& runtimeArguments);

	void scaleExpandPhaseTo(int numNodes, int numGpusPerNode, const std::map<std::string, std::string>& runtimeArguments);

	void scaleTo(int numNodes, int numGpusPerNode, const std::map<std::string, std::string>& runtimeArguments);

	void advance(int completedPhases, int remainingIterations);

	void complete();
};


#endif //ELASTISIM_WORKLOAD_H
