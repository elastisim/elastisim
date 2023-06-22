/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_SEQUENCETASK_H
#define ELASTISIM_SEQUENCETASK_H


#include <memory>
#include <list>

#include "Task.h"


class SequenceTask : public Task {

private:
	std::deque<std::unique_ptr<Task>> tasks;

	static double logTaskStart(const Task* task, int iterations);

	static double logTaskEnd(const Task* task, double start);

	static double logIterationStart(int iterations, int i);

	static void logIterationEnd(int iterations, int i, double start);

public:
	SequenceTask(const std::string& name, const std::string& iterations, bool synchronized,
				 std::deque<std::unique_ptr<Task>> tasks);

	void execute(const Node* node, const Job* job,
				 const std::vector<Node*>& nodes, int rank, simgrid::s4u::BarrierPtr barrier) const override;

	void scaleTo(int numNodes, int numGpusPerNode) override;
};


#endif //ELASTISIM_SEQUENCETASK_H
