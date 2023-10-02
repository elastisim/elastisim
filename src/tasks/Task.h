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
#include <optional>
#include <simgrid/s4u.hpp>

class Node;

class Job;

enum VectorPattern {
	ALL_RANKS,
	ROOT_ONLY,
	EVEN_RANKS,
	ODD_RANKS,
	UNIFORM,
	VECTOR
};

enum MatrixPattern {
	ALL_TO_ALL,
	GATHER,
	SCATTER,
	MASTER_WORKER,
	RING,
	RING_CLOCKWISE,
	RING_COUNTER_CLOCKWISE,
	MATRIX
};

class Task {

private:
	const std::string name;
	const std::string iterationModel;
	int iterations;
	const bool synchronized;

public:
	Task(std::string name, std::string iterations, bool synchronized);

	virtual ~Task();

	[[nodiscard]] const std::string& getName() const;

	[[nodiscard]] int getIterations() const;

	void updateIterations(int numNodes, int numGpusPerNode);

	[[nodiscard]] bool isSynchronized() const;

	[[nodiscard]] virtual bool isAsynchronous() const;

	virtual void execute(const Node* node, const Job* job, const std::vector<Node*>& nodes, int rank,
						 simgrid::s4u::BarrierPtr barrier) const = 0;

	[[nodiscard]] virtual std::vector<simgrid::s4u::ActivityPtr>
	executeAsync(const Node* node, const Job* job, const std::vector<Node*>& nodes, int rank) const;

	virtual void scaleTo(int numNodes, int numGpusPerNode, const std::map<std::string, std::string>& runtimeArguments);
};


#endif //ELASTISIM_TASK_H
