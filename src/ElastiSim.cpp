/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "ElastiSim.h"

#include <simgrid/s4u.hpp>
#include <xbt/parse_units.hpp>
#include <sstream>

#include "PlatformManager.h"
#include "SimulationEngine.h"
#include "Scheduler.h"
#include "Node.h"
#include "Sensing.h"
#include "JobSubmitter.h"
#include "Configuration.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(ElastiSim, "Messages within ElastiSim");

std::string ElastiSim::getPropertyIfExists(const char* property) {
	if (property) {
		return {property};
	} else {
		return {};
	}
}

void ElastiSim::startSimulation(int argc, char* argv[]) {

	Configuration::init(argv[1]);

	simgrid::s4u::Engine engine(&argc, argv);
	simgrid::s4u::Engine::set_config("host/model:ptask_L07");
	engine.load_platform(Configuration::get("platform_file"));

	std::ofstream nodeUtilization(Configuration::get("node_utilization"));
	nodeUtilization << "Time,Node,State,Running jobs,Expected jobs" << std::endl;

	const std::vector<s4u_Host*>& hosts = engine.get_all_hosts();
	std::vector<s4u_Host*> filteredHosts;
	std::vector<s4u_Host*> filteredPfs;

	s4u_Host* masterHost = nullptr;
	for (auto& host: hosts) {
		if (getPropertyIfExists(host->get_property("batch_system")) == "true") {
			if (!masterHost) {
				masterHost = host;
			} else {
				xbt_die("Batch system host already specified");
			}
		} else if (getPropertyIfExists(host->get_property("pfs_host")) == "true") {
			filteredPfs.push_back(host);
		} else {
			filteredHosts.push_back(host);
		}
	}

	std::vector<std::unique_ptr<Node>> nodes;
	int id = 0;
	for (auto& host: filteredHosts) {

		const std::string& hostname = host->get_name();

		std::vector<s4u_Host*> pfsTargets;
		if (host->get_property("pfs_targets")) {
			std::stringstream stream(host->get_property("pfs_targets"));
			while (stream.good()) {
				std::string pfsHostname;
				getline(stream, pfsHostname, ',');
				pfsTargets.push_back(engine.host_by_name(pfsHostname));
			}
		} else {
			pfsTargets = filteredPfs;
		}

		int numGpus = 0;
		long flopsPerGpu = 0;
		long gpuToGpuBandwidth = 0;
		if (host->get_property("num_gpus")) {
			numGpus = std::stoi(host->get_property("num_gpus"));
			flopsPerGpu = (long) xbt_parse_get_speed("", 0, host->get_property("flops_per_gpu"), "");
			if (numGpus > 1) {
				gpuToGpuBandwidth = (long) xbt_parse_get_bandwidth("", 0, host->get_property("gpu_to_gpu_bw"), "");
			}
		}
		std::vector<std::unique_ptr<Gpu>> gpus;
		gpus.reserve(numGpus);
		for (int i = 0; i < numGpus; ++i) {
			gpus.push_back(std::make_unique<Gpu>(i, flopsPerGpu, host));
		}

		if (getPropertyIfExists(host->get_property("node_local_bb")) == "true") {
			s4u_Disk* disk = host->create_disk("BurstBuffer@" + hostname, host->get_property("bb_read_bw"),
											   host->get_property("bb_write_bw"));
			disk->seal();
			if (getPropertyIfExists(host->get_property("wide_striping")) == "true") {

				double flopsPerByte = 0;
				if (host->get_property("flops_per_byte")) {
					flopsPerByte = xbt_parse_get_speed("", 0, host->get_property("flops_per_byte"), "");
				}
				nodes.emplace_back(
						std::make_unique<Node>(id++, COMPUTE_NODE_WITH_WIDE_STRIPED_BB, host, disk, pfsTargets,
											   flopsPerByte, std::move(gpus), gpuToGpuBandwidth, nodeUtilization));
			} else {
				nodes.emplace_back(
						std::make_unique<Node>(id++, COMPUTE_NODE_WITH_BB, host, disk, pfsTargets, 0, std::move(gpus),
											   gpuToGpuBandwidth, nodeUtilization));
			}
		} else {
			nodes.emplace_back(
					std::make_unique<Node>(id++, COMPUTE_NODE, host, nullptr, pfsTargets, 0, std::move(gpus),
										   gpuToGpuBandwidth, nodeUtilization));
		}
		Node* node = nodes.back().get();
		s4u_Actor::create(hostname, host, [node] { node->act(); });

	}

	PlatformManager::init(std::move(nodes));

	if (!masterHost) {
		masterHost = hosts.front();
	}
	s4u_Actor::create("JobSubmitter", masterHost, JobSubmitter());
	s4u_Actor::create("SimulationEngine", masterHost, SimulationEngine());
	s4u_Actor::create("Scheduler", masterHost, Scheduler(masterHost));
	if (Configuration::getBoolIfExists("sensing")) {
		s4u_Actor::create("Sensing", masterHost, Sensing());
	}

	engine.run();
}
