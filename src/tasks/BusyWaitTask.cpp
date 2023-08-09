/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "BusyWaitTask.h"

#include <simgrid/s4u.hpp>
#include "Node.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(BusyWaitTask, "Messages within the busy wait task");

BusyWaitTask::BusyWaitTask(const std::string& name, const std::string& iterations, bool synchronized,
						   const std::optional<std::vector<double>>& delays,
						   const std::optional<std::string>& delayModel, VectorPattern delayPattern) :
		DelayTask(name, iterations, synchronized, delays, delayModel, delayPattern) {}

void BusyWaitTask::execute(const Node* node, const Job* job, const std::vector<Node*>& nodes, int rank,
						   simgrid::s4u::BarrierPtr barrier) const {
	XBT_INFO("Waiting %f seconds", delays[rank]);
	node->getHost()->execute(delays[rank] * node->getHost()->get_speed());
}
