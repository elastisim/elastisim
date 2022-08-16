/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_BUSYWAITTASK_H
#define ELASTISIM_BUSYWAITTASK_H


#include "DelayTask.h"

class BusyWaitTask : public DelayTask {

public:
	BusyWaitTask(const std::string& name, const std::string& iterations, bool synchronized,
				 const std::vector<double>& delays, VectorPattern delayPattern);

	BusyWaitTask(const std::string& name, const std::string& iterations, bool synchronized,
				 const std::string& delayModel, VectorPattern delayPattern);

	void execute(const Node* node, const Job* job,
				 const std::vector<Node*>& nodes, int rank, simgrid::s4u::BarrierPtr barrier) const override;

};


#endif //ELASTISIM_BUSYWAITTASK_H
