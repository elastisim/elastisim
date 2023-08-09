/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "Sensing.h"

#include <fstream>
#include <simgrid/s4u.hpp>

#include "PlatformManager.h"
#include "Node.h"
#include "Configuration.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(Sensing, "Messages within the Sensing actor");

Sensing::Sensing() : sensingInterval(Configuration::get("sensing_interval")) {}

void Sensing::operator()() const {

	s4u_Actor::self()->daemonize();

	simgrid::s4u::Engine* engine = simgrid::s4u::Engine::get_instance();
	std::ofstream cpuUtilization(Configuration::get("cpu_utilization"));
	std::ofstream networkActivity(Configuration::get("network_activity"));
	std::ofstream pfsUtilization(Configuration::get("pfs_utilization"));
	std::ofstream gpuUtilization(Configuration::get("gpu_utilization"));

	std::vector<Node*> nodes;
	std::vector<simgrid::s4u::Link*> links;

	std::string nodeNames;
	for (const auto& node: PlatformManager::getComputeNodes()) {
		nodeNames += node->getHostName() + ",";
		nodes.push_back(node);
	}
	nodeNames.pop_back();

	std::vector<std::string> pfsReadLinks = Configuration::get("pfs_read_links");
	std::vector<std::string> pfsWriteLinks = Configuration::get("pfs_write_links");
	for (const auto& link: engine->get_all_links()) {
		std::string linkName = link->get_name();
		if (linkName.find("loopback") == std::string::npos &&
			linkName.find("_limiter") == std::string::npos &&
			std::find(std::begin(pfsReadLinks), std::end(pfsReadLinks), linkName) == std::end(pfsReadLinks) &&
			std::find(std::begin(pfsWriteLinks), std::end(pfsWriteLinks), linkName) == std::end(pfsWriteLinks)) {
			links.push_back(link);
		}
	}
	size_t numberOfLinks = links.size();

	double pfsReadBandwidth = PlatformManager::getPfsReadBandwidth();
	double pfsWriteBandwidth = PlatformManager::getPfsWriteBandwidth();

	cpuUtilization << "Time," << nodeNames << std::endl;
	networkActivity << "Time,Utilization" << std::endl;
	pfsUtilization << "Time,Read,Write,Read (rel.),Write (rel.)" << std::endl;
	gpuUtilization << "Time," << nodeNames << std::endl;

	double time;
	double networkUsage;
	double pfsRead;
	double pfsWrite;
	std::string nodeCpuUtilizations;
	std::string nodeGpuUtilizations;

	while (simgrid::s4u::this_actor::get_host()->is_on()) {

		time = simgrid::s4u::Engine::get_clock();

		cpuUtilization << time << ",";
		gpuUtilization << time << ",";
		for (const auto& node: nodes) {
			s4u_Host* host = node->getHost();
			nodeCpuUtilizations += std::to_string(host->get_load() / host->get_speed()) + ",";
			std::vector<const Gpu*> gpus = node->getGpus();
			double totalGpuUtilization = 0;
			for (const auto& gpu: gpus) {
				totalGpuUtilization += gpu->getUtilization();
			}
			nodeGpuUtilizations += gpus.empty() ? "0," : std::to_string(totalGpuUtilization / gpus.size()) + ",";
		}
		nodeCpuUtilizations.pop_back();
		cpuUtilization << nodeCpuUtilizations << std::endl;
		nodeCpuUtilizations.clear();
		nodeGpuUtilizations.pop_back();
		gpuUtilization << nodeGpuUtilizations << std::endl;
		nodeGpuUtilizations.clear();

		networkUsage = 0;
		for (const auto& link: links) {
			networkUsage += link->get_usage() / link->get_bandwidth();
		}
		networkActivity << time << ",";
		networkActivity << networkUsage / numberOfLinks << std::endl;

		pfsRead = PlatformManager::getPfsReadUtilization();
		pfsWrite = PlatformManager::getPfsWriteUtilization();

		pfsUtilization << time << ",";
		pfsUtilization << pfsRead << ",";
		pfsUtilization << pfsWrite << ",";
		pfsUtilization << pfsRead / pfsReadBandwidth << ",";
		pfsUtilization << pfsWrite / pfsWriteBandwidth << std::endl;

		simgrid::s4u::this_actor::sleep_for(sensingInterval);
	}
}
