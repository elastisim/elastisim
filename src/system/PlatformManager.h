/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_PLATFORMMANAGER_H
#define ELASTISIM_PLATFORMMANAGER_H


#include <vector>
#include <memory>
#include <simgrid/s4u/Engine.hpp>

class Node;

class PlatformManager {

private:
	static std::vector<std::unique_ptr<Node>> nodes;
	static std::vector<Node*> computeNodes;
	static std::vector<Node*> modifiedComputeNodes;
	static std::vector<s4u_Link*> pfsReadLinks;
	static std::vector<s4u_Link*> pfsWriteLinks;
	static double pfsReadBandwidth;
	static double pfsWriteBandwidth;
	static bool initialized;

public:
	static void init(std::vector<std::unique_ptr<Node>> initialNodes);

	[[nodiscard]] static const std::vector<Node*>& getComputeNodes();

	[[nodiscard]] static const std::vector<Node*>& getModifiedComputeNodes();

	static void addModifiedComputeNode(Node* node);

	static void clearModifiedComputeNodes();

	[[nodiscard]] static double getPfsReadUtilization();

	[[nodiscard]] static double getPfsWriteUtilization();

	[[nodiscard]] static double getPfsReadBandwidth();

	[[nodiscard]] static double getPfsWriteBandwidth();

};


#endif //ELASTISIM_PLATFORMMANAGER_H
