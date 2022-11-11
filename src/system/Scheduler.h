/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_SCHEDULER_H
#define ELASTISIM_SCHEDULER_H

#include "Job.h"
#include <memory>
#include "SchedulingInterface.h"

class Node;

class Job;

class Scheduler {

private:
	s4u_Host* masterHost;
	double schedulingInterval;
	double minSchedulingInterval;
	double lastInvocation;
	bool scheduleOnJobSubmit;
	bool scheduleOnJobFinalize;
	std::vector<Job*> jobQueue;
	std::map<Job*, simgrid::s4u::ActorPtr> walltimeMonitors;
	std::map<Job*, std::set<Node*>> assignedNodes;
	int currentJobId;

	void schedule();

	void handleJobSubmit(Job* job);

	void handleProcessedWorkload(Job* job, Node* node);

	void forwardJobKill(Job* job, bool exceededWalltime);

	void forwardJobAllocation(Job* job);

	void handleSchedulingPoint(Job* job, Node* node, int completedPhases, int remainingIterations);

public:
	explicit Scheduler(s4u_Host* masterHost);

	void operator()();

};


#endif //ELASTISIM_SCHEDULER_H
