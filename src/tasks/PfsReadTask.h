/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_PFSREADTASK_H
#define ELASTISIM_PFSREADTASK_H


#include <vector>
#include "IoTask.h"

class PfsReadTask : public IoTask {

public:
	PfsReadTask(const std::string& name, const std::string& iterations, bool synchronized, bool asynchronous,
				const std::vector<double>& ioSizes, VectorPattern ioPattern);

	PfsReadTask(const std::string& name, const std::string& iterations, bool synchronized, bool asynchronous,
				const std::string& ioModel, VectorPattern ioPattern);

	void execute(const Node* node, const Job* job,
				 const std::vector<Node*>& nodes, int rank, simgrid::s4u::BarrierPtr barrier) const override;

	std::vector<simgrid::s4u::ActivityPtr>
	executeAsync(const Node* node, const Job* job,
				 const std::vector<Node*>& nodes, int rank) const override;

};


#endif //ELASTISIM_PFSREADTASK_H
