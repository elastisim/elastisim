/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_PHASE_H
#define ELASTISIM_PHASE_H


#include <deque>
#include <memory>

class Task;

class Phase {

private:
	std::deque<std::unique_ptr<Task>> tasks;
	std::deque<Task*> taskPointers;
	int iterations;
	bool schedulingPoint;
	bool finalSchedulingPoint;
	bool barrier;

public:
	Phase(std::deque<std::unique_ptr<Task>> tasks, int iterations, bool schedulingPoint, bool finalSchedulingPoint,
		  bool barrier);

	const std::deque<Task*>& getTasks() const;

	int getIterations() const;

	void setIterations(int iterations);

	bool hasSchedulingPoint() const;

	bool hasFinalSchedulingPoint() const;

	bool hasBarrier() const;

	void scaleTo(int numNodes, int numGpusPerNode);
};


#endif //ELASTISIM_PHASE_H
