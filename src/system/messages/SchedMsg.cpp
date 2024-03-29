/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "SchedMsg.h"

SchedMsg::SchedMsg(SchedEventType type) : type(type), numberOfNodes(-1) {}

SchedMsg::SchedMsg(SchedEventType type, Job* job) : type(type), job(job), numberOfNodes(-1) {}

SchedMsg::SchedMsg(SchedEventType type, Job* job, int numberOfNodes) :
		type(type), job(job), numberOfNodes(numberOfNodes) {}

SchedEventType SchedMsg::getType() const {
	return type;
}

Job* SchedMsg::getJob() const {
	return job;
}

int SchedMsg::getNumberOfNodes() const {
	return numberOfNodes;
}
