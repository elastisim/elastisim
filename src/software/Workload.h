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

class Task;

class Phase;

class Workload {

private:
	std::unique_ptr<Phase> initPhase;
	std::unique_ptr<Phase> reconfigurationPhase;
	std::unique_ptr<Phase> expansionPhase;
	std::deque<std::unique_ptr<Phase>> phases;
	std::deque<Phase*> phasePointers;
	int totalPhaseCount;
	int completedPhases;

public:
	Workload(std::unique_ptr<Phase> initPhase, std::unique_ptr<Phase> reconfigurationPhase,
			 std::unique_ptr<Phase> expansionPhase, std::deque<std::unique_ptr<Phase>> phases);

	const Phase* getInitPhase() const;

	const Phase* getReconfigurationPhase() const;

	const Phase* getExpansionPhase() const;

	const std::deque<Phase*>& getPhases() const;

	int getTotalPhaseCount() const;

	int getCompletedPhases() const;

	void scaleInitPhaseTo(int numNodes, int numGpusPerNode);

	void scaleReconfigurationPhaseTo(int numNodes, int numGpusPerNode);

	void scaleExpandPhaseTo(int numNodes, int numGpusPerNode);

	void scaleTo(int numNodes, int numGpusPerNode);

	void advance(int completedPhases, int remainingIterations);

	void complete();
};


#endif //ELASTISIM_WORKLOAD_H
