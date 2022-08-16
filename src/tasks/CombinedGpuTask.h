/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_COMBINEDGPUTASK_H
#define ELASTISIM_COMBINEDGPUTASK_H


#include "CombinedTask.h"

class CombinedGpuTask : public CombinedTask {

private:
	std::vector<double> intraNodeCommunications;
	std::vector<double> interNodeCommunications;

public:
	CombinedGpuTask(const std::string& name, const std::string& iterations, bool synchronized,
					const std::vector<double>& flops,
					std::vector<double> intraNodeCommunications, std::vector<double> interNodeCommunications);

	CombinedGpuTask(const std::string& name, const std::string& iterations, bool synchronized,
					const std::vector<double>& flops,
					const std::string& communicationModel, MatrixPattern communicationPattern);

	CombinedGpuTask(const std::string& name, const std::string& iterations, bool synchronized,
					const std::string& computationModel,
					VectorPattern computationPattern, std::vector<double> intraNodeCommunications,
					std::vector<double> interNodeCommunications);

	CombinedGpuTask(const std::string& name, const std::string& iterations, bool synchronized,
					const std::string& computationModel,
					VectorPattern computationPattern, const std::string& communicationModel,
					MatrixPattern communicationPattern);

	void execute(const Node* node, const Job* job,
				 const std::vector<Node*>& nodes, int rank, simgrid::s4u::BarrierPtr barrier) const override;

	void scaleTo(int numNodes, int numGpusPerNode) override;

};


#endif //ELASTISIM_COMBINEDGPUTASK_H
