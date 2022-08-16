/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "SchedulingInterface.h"
#include "Job.h"
#include "Node.h"
#include "PlatformManager.h"
#include "Configuration.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(SchedulingInterface, "Messages within the scheduling interface");

zmq::context_t SchedulingInterface::context;
zmq::socket_t SchedulingInterface::socket(context, zmq::socket_type::pair);

void SchedulingInterface::invokeScheduling(const std::vector<Job*>& jobQueue) {
	std::vector<Node*> nodes = PlatformManager::getComputeNodes();
	nlohmann::json message;
	message["code"] = ZMQ_INVOKE_SCHEDULING;
	message["time"] = simgrid::s4u::Engine::get_clock();
	message["jobs"] = nlohmann::json::array();
	for (auto& job: jobQueue) {
		message["jobs"].push_back(job->toJson());
	}
	message["nodes"] = nlohmann::json::array();
	for (auto& node: nodes) {
		message["nodes"].push_back(node->toJson());
	}
	message["pfs_read_bw"] = PlatformManager::getPfsReadBandwidth();
	message["pfs_write_bw"] = PlatformManager::getPfsWriteBandwidth();
	message["pfs_read_utilization"] = PlatformManager::getPfsReadUtilization();
	message["pfs_write_utilization"] = PlatformManager::getPfsWriteUtilization();
	socket.send(zmq::buffer(message.dump()));
}

void SchedulingInterface::init() {
	socket = zmq::socket_t(context, zmq::socket_type::pair);
	socket.bind(Configuration::get("zmq_url"));
}

void SchedulingInterface::handleSchedule(const nlohmann::json& jsonJobs, const std::vector<Job*>& jobQueue) {
	const std::vector<Node*>& nodes = PlatformManager::getComputeNodes();
	for (auto& jsonJob: jsonJobs) {
		Job* job = jobQueue[jsonJob["id"]];
		if (jsonJob["kill_flag"]) {
			job->setState(TO_BE_KILLED);
		} else {
			job->clearAssignedNodes();
			for (auto& jsonNodeId: jsonJob["assigned_node_ids"]) {
				job->assignNode(nodes[jsonNodeId]);
			}
			if (job->getType() != RIGID) {
				job->assignNumGpusPerNode(jsonJob["assigned_num_gpus_per_node"]);
			}
			job->checkConfigurationValidity();
			job->updateState();
		}
	}
}

void SchedulingInterface::schedule(const std::vector<Job*>& jobQueue) {

	invokeScheduling(jobQueue);

	zmq::message_t message;
	nlohmann::json json;

	socket.recv(message, zmq::recv_flags::none);
	json = nlohmann::json::parse(message.to_string());
	if (json["code"] == ZMQ_SCHEDULED) {
		handleSchedule(json["jobs"], jobQueue);
	} else {
		xbt_die("Unknown message code from scheduling algorithm");
	}
}

void SchedulingInterface::finalize() {
	nlohmann::json message;
	message["code"] = ZMQ_FINALIZE;
	socket.send(zmq::buffer(message.dump()));
	socket.close();
}
