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

#define EPSILON 0.001

#include "Job.h"
#include <memory>

class Node;

class Job;

enum InvocationType {
	INVOKE_PERIODIC = 0,
	INVOKE_JOB_SUBMIT = 1,
	INVOKE_JOB_COMPLETED = 2,
	INVOKE_JOB_KILLED = 3,
	INVOKE_SCHEDULING_POINT = 4,
	INVOKE_EVOLVING_REQUEST = 5
};

class Scheduler {

private:
	s4u_Host* masterHost;
	const double schedulingInterval;
	const double minSchedulingInterval;
	double lastInvocation;
	const bool scheduleOnJobSubmit;
	const bool scheduleOnJobFinalize;
	const bool scheduleOnSchedulingPoint;
	const double gracePeriod;
	std::vector<Job*> jobQueue;
	std::vector<Job*> modifiedJobs;
	std::map<Job*, s4u_Actor*> walltimeMonitors;
	std::map<Job*, std::set<Node*>> assignedNodes;
	int currentJobId;

	void schedule(InvocationType invocationType, Job* requestingJob = nullptr, int numberOfNodes = -1);

	void handleJobSubmit(Job* job);

	void handleProcessedWorkload(Job* job);

	void forwardJobKill(Job* job, bool exceededWalltime);

	void forwardJobAllocation(Job* job);

	void handleReconfiguration(Job* job);

	void handleSchedulingPoint(Job* job);

	void handleEvolvingRequest(Job* job, int numberOfnodes);

	void checkConfigurationValidity() const;

public:
	explicit Scheduler(s4u_Host* masterHost);

	void operator()();

};


#endif //ELASTISIM_SCHEDULER_H
