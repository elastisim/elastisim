/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_IOTASK_H
#define ELASTISIM_IOTASK_H

#include <list>
#include "Task.h"

class IoTask : public Task {

private:
	bool asynchronous;

protected:
	std::vector<double> ioSizes;
	std::string ioModel;
	VectorPattern ioPattern;

public:
	IoTask(const std::string& name, const std::string& iterations, bool synchronized, bool asynchronous, std::vector<double> ioSizes,
		   VectorPattern ioPattern);

	IoTask(const std::string& name, const std::string& iterations, bool synchronized, bool asynchronous, std::string ioModel,
		   VectorPattern ioPattern);

	bool isAsynchronous() const override;

	void execute(const Node* node, const Job* job,
				 const std::vector<Node*>& nodes, int rank,
				 simgrid::s4u::BarrierPtr barrier) const override = 0;

	void scaleTo(int numNodes, int numGpusPerNode) override;

};


#endif //ELASTISIM_IOTASK_H
