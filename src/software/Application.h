/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_APPLICATION_H
#define ELASTISIM_APPLICATION_H

#include <memory>
#include <list>
#include <simgrid/s4u.hpp>

class Phase;

class Job;

class Node;

class Task;

class Application {

private:
	Node* node;
	Job* job;
	int rank;

	static void waitForAsyncActivities(const std::vector<simgrid::s4u::ActivityPtr>& asyncActivities);

public:
	Application(Node* node, Job* job, int rank);

	static void executeOneTimePhase(const Phase* phase, const Node* node,
									const Job* job, const std::vector<Node*>& nodes,
									int rank, const simgrid::s4u::BarrierPtr& barrier);

	void operator()();
};


#endif //ELASTISIM_APPLICATION_H
