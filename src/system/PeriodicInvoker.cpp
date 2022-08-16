/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "PeriodicInvoker.h"

#include <simgrid/s4u.hpp>
#include "SchedMsg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(PeriodicInvoker, "Messages within the PeriodicInvoker");

PeriodicInvoker::PeriodicInvoker(double schedulingInterval) : schedulingInterval(schedulingInterval) {}

void PeriodicInvoker::operator()() const {
	s4u_Actor::self()->daemonize();
	s4u_Mailbox* mailboxScheduler = s4u_Mailbox::by_name("Scheduler");
	while (simgrid::s4u::this_actor::get_host()->is_on()) {
		simgrid::s4u::this_actor::sleep_for(schedulingInterval);
		mailboxScheduler->put(new SchedMsg(INVOKE_SCHEDULING), 0);
	}
}
