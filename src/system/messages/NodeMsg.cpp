/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "NodeMsg.h"

NodeMsg::NodeMsg(NodeEvent type, Job* job) : type(type), job(job), rank(-1) {}

NodeMsg::NodeMsg(NodeEvent type) : type(type), rank(-1) {}

NodeMsg::NodeMsg(NodeEvent type, Job* job, int rank, simgrid::s4u::BarrierPtr barrier)
		: type(type), job(job), rank(rank), barrier(std::move(barrier)) {}

NodeMsg::NodeMsg(NodeEvent type, Job* job, int completedPhases, int remainingIterations)
		: type(type), job(job), completedPhases(completedPhases),
		  remainingIterations(remainingIterations) {}

NodeMsg::NodeMsg(NodeEvent type, Job* job, int rank, int expandRank,
				 simgrid::s4u::BarrierPtr barrier, simgrid::s4u::BarrierPtr expandBarrier) :
		type(type), job(job), rank(rank), expandRank(expandRank), barrier(std::move(barrier)),
		expandBarrier(std::move(expandBarrier)) {}

NodeEvent NodeMsg::getType() const {
	return type;
}

Job* NodeMsg::getJob() const {
	return job;
}

int NodeMsg::getRank() const {
	return rank;
}

int NodeMsg::getExpandRank() const {
	return expandRank;
}

const simgrid::s4u::BarrierPtr& NodeMsg::getBarrier() const {
	return barrier;
}

const simgrid::s4u::BarrierPtr& NodeMsg::getExpandBarrier() const {
	return expandBarrier;
}

int NodeMsg::getCompletedPhases() const {
	return completedPhases;
}

int NodeMsg::getRemainingIterations() const {
	return remainingIterations;
}
