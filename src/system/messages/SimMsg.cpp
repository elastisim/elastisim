/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "SimMsg.h"
#include "Job.h"
#include "Workload.h"
#include "Phase.h"
#include "Task.h"

SimMsg::SimMsg(SimEventType type, size_t numberOfJobs) : type(type), numberOfJobs(numberOfJobs) {}

SimMsg::SimMsg(SimEventType type, std::unique_ptr<Job> job) : type(type), numberOfJobs(-1), job(std::move(job)) {}

SimEventType SimMsg::getType() const {
	return type;
}

size_t SimMsg::getNumberOfJobs() const {
	return numberOfJobs;
}

std::unique_ptr<Job> SimMsg::getJob() {
	return std::move(job);
}
