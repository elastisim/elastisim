/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_BURSTBUFFERWRITETASK_H
#define ELASTISIM_BURSTBUFFERWRITETASK_H


#include <vector>
#include <list>
#include "IoTask.h"

class BurstBufferWriteTask : public IoTask {

public:
	BurstBufferWriteTask(const std::string& name, const std::string& iterations, bool synchronized, bool asynchronous,
						 const std::vector<double>& ioSizes, VectorPattern ioPattern);

	BurstBufferWriteTask(const std::string& name, const std::string& iterations, bool synchronized, bool asynchronous,
						 const std::string& ioModel, VectorPattern ioPattern);

	void execute(const Node* node, const Job* job,
				 const std::vector<Node*>& nodes, int rank, simgrid::s4u::BarrierPtr barrier) const override;

};


#endif //ELASTISIM_BURSTBUFFERWRITETASK_H
