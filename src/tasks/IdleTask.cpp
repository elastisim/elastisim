/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "IdleTask.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(IdleTask, "Messages within the busy wait task");

IdleTask::IdleTask(const std::string& name, const std::string& iterations, bool synchronized,
				   const std::optional<std::vector<double>>& delays, const std::optional<std::string>& delayModel,
				   VectorPattern delayPattern) :
		DelayTask(name, iterations, synchronized, delays, delayModel, delayPattern) {}

void IdleTask::execute(const Node* node, const Job* job, const std::vector<Node*>& nodes, int rank,
					   simgrid::s4u::BarrierPtr barrier) const {
	XBT_INFO("Idling %f seconds", delays[rank]);
	simgrid::s4u::this_actor::sleep_for(delays[rank]);
}
