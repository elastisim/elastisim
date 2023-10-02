/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_COMBINEDTASK_H
#define ELASTISIM_COMBINEDTASK_H


#include "Task.h"

class CombinedTask : public Task {

protected:
	std::vector<double> flops;
	const std::string computationModel;
	const VectorPattern computationPattern;
	const std::string communicationModel;
	const MatrixPattern communicationPattern;

public:
	CombinedTask(const std::string& name, const std::string& iterations, bool synchronized,
				 std::optional<std::vector<double>> flops, std::optional<std::string> computationModel,
				 VectorPattern computationPattern, std::optional<std::string> communicationModel,
				 MatrixPattern communicationPattern);

	void execute(const Node* node, const Job* job, const std::vector<Node*>& nodes, int rank,
				 simgrid::s4u::BarrierPtr barrier) const override = 0;

	void scaleTo(int numNodes, int numGpusPerNode, const std::map<std::string, std::string>& runtimeArguments) override;

};


#endif //ELASTISIM_COMBINEDTASK_H
