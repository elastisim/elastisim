/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "WalltimeMonitor.h"

#include <simgrid/s4u.hpp>
#include "Job.h"
#include "SchedMsg.h"

WalltimeMonitor::WalltimeMonitor(Job* job, double gracePeriod) : job(job), gracePeriod(gracePeriod) {}

void WalltimeMonitor::operator()() {
	s4u_Mailbox* mailboxScheduler = s4u_Mailbox::by_name("Scheduler");
	simgrid::s4u::this_actor::sleep_until(job->getStartTime() + job->getWalltime() + gracePeriod);
	mailboxScheduler->put(new SchedMsg(WALLTIME_EXCEEDED, job), 0);
}

