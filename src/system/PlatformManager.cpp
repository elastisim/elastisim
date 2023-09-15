/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "PlatformManager.h"

#include <utility>
#include <xbt/asserts.h>
#include "Node.h"
#include "Configuration.h"

std::vector<std::unique_ptr<Node>> PlatformManager::nodes;
std::vector<Node*> PlatformManager::computeNodes;
std::vector<Node*> PlatformManager::modifiedComputeNodes;
std::vector<s4u_Link*> PlatformManager::pfsReadLinks;
std::vector<s4u_Link*> PlatformManager::pfsWriteLinks;
double PlatformManager::pfsReadBandwidth = 0;
double PlatformManager::pfsWriteBandwidth = 0;

bool PlatformManager::initialized = false;

void PlatformManager::init(std::vector<std::unique_ptr<Node>> initialNodes) {
	if (!initialized) {
		nodes = std::move(initialNodes);
		for (const auto& node: nodes) {
			computeNodes.push_back(node.get());
		}
		simgrid::s4u::Engine* engine = simgrid::s4u::Engine::get_instance();

		for (const auto& linkName: Configuration::get("pfs_read_links")) {
			s4u_Link* link = engine->link_by_name(linkName);
			pfsReadBandwidth += engine->link_by_name(linkName)->get_bandwidth();
			pfsReadLinks.push_back(link);
		}

		for (const auto& linkName: Configuration::get("pfs_write_links")) {
			s4u_Link* link = engine->link_by_name(linkName);
			pfsWriteBandwidth += engine->link_by_name(linkName)->get_bandwidth();
			pfsWriteLinks.push_back(link);
		}

		initialized = true;
	} else {
		xbt_die("PlatformManager already initialized");
	}
}

const std::vector<Node*>& PlatformManager::getComputeNodes() {
	return computeNodes;
}

const std::vector<Node*>& PlatformManager::getModifiedComputeNodes() {
	return modifiedComputeNodes;
}

void PlatformManager::addModifiedComputeNode(Node* node) {
	modifiedComputeNodes.push_back(node);
}

void PlatformManager::clearModifiedComputeNodes() {
	modifiedComputeNodes.clear();
}

double PlatformManager::getPfsReadUtilization() {
	double pfsRead = 0;
	for (const auto& link: pfsReadLinks) {
		pfsRead += link->get_load();
	}
	return pfsRead;
}

double PlatformManager::getPfsWriteUtilization() {
	double pfsWrite = 0;
	for (const auto& link: pfsWriteLinks) {
		pfsWrite += link->get_load();
	}
	return pfsWrite;
}

double PlatformManager::getPfsReadBandwidth() {
	return pfsReadBandwidth;
}

double PlatformManager::getPfsWriteBandwidth() {
	return pfsWriteBandwidth;
}
