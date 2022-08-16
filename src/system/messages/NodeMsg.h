/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_NODEMSG_H
#define ELASTISIM_NODEMSG_H


#include <simgrid/s4u.hpp>

class Job;

class Task;

enum NodeEvent {
	NODE_ALLOCATE,
	NODE_CONTINUE,
	NODE_RECONFIGURE,
	NODE_EXPAND,
	NODE_KILL,
	NODE_DEALLOCATE,
	WORKLOAD_COMPLETED,
	AT_SCHEDULING_POINT,
	NODE_FINALIZE
};

class NodeMsg {

private:
	NodeEvent type;
	Job* job;
	int rank;
	int expandRank;
	simgrid::s4u::BarrierPtr barrier;
	simgrid::s4u::BarrierPtr expandBarrier;
	int completedPhases;
	int remainingIterations;

public:
	NodeMsg(NodeEvent type, Job* job, int rank, simgrid::s4u::BarrierPtr barrier);

	NodeMsg(NodeEvent type, Job* job);

	NodeMsg(NodeEvent type, Job* job, int completedPhases, int remainingIterations);

	NodeMsg(NodeEvent type, Job* job, int rank, int expandRank, simgrid::s4u::BarrierPtr barrier,
			simgrid::s4u::BarrierPtr expandBarrier);

	explicit NodeMsg(NodeEvent type);

	NodeEvent getType() const;

	Job* getJob() const;

	int getRank() const;

	int getExpandRank() const;

	const simgrid::s4u::BarrierPtr& getBarrier() const;

	const simgrid::s4u::BarrierPtr& getExpandBarrier() const;

	int getCompletedPhases() const;

	int getRemainingIterations() const;

};


#endif //ELASTISIM_NODEMSG_H
