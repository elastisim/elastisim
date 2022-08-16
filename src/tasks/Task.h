/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_TASK_H
#define ELASTISIM_TASK_H

#include <vector>
#include <simgrid/s4u.hpp>

class Node;

class Job;

enum VectorPattern {
	VECTOR,
	UNIFORM,
	EVEN,
	ODD,
	TOTAL
};

enum MatrixPattern {
	MATRIX,
	ALL_TO_ALL,
	RING,
	RING_CLOCKWISE,
	RING_COUNTER_CLOCKWISE,
	MASTER_WORKER
};

class Task {

private:
	std::string name;
	std::string iterationModel;
	int iterations;
	bool synchronized;

public:
	Task(std::string name, std::string iterations, bool synchronized);

	virtual ~Task();

	const std::string& getName() const;

	int getIterations() const;

	void updateIterations(int numNodes, int numGpusPerNode);

	bool isSynchronized() const;

	virtual bool isAsynchronous() const;

	virtual void execute(const Node* node, const Job* job,
						 const std::vector<Node*>& nodes, int rank, simgrid::s4u::BarrierPtr barrier) const = 0;

	virtual std::vector<simgrid::s4u::ActivityPtr>
	executeAsync(const Node* node, const Job* job, const std::vector<Node*>& nodes, int rank) const;

	virtual void scaleTo(int numNodes, int numGpusPerNode);
};


#endif //ELASTISIM_TASK_H
