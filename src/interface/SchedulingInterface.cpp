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
const bool SchedulingInterface::forwardIoInformation = Configuration::getBoolIfExists("forward_io_information");

void SchedulingInterface::invokeScheduling(InvocationType invocationType, const std::vector<Job*>& modifiedJobs,
										   const Job* requestingJob, int numberOfNodes) {
	std::vector<Node*> nodes = PlatformManager::getModifiedComputeNodes();
	nlohmann::json message;
	message["code"] = ZMQ_INVOKE_SCHEDULING;
	message["time"] = simgrid::s4u::Engine::get_clock();
	message["invocation_type"] = invocationType;
	if (invocationType != INVOKE_PERIODIC) {
		message["job_id"] = requestingJob->getId();
		if (invocationType == INVOKE_EVOLVING_REQUEST) {
			message["evolving_request"] = numberOfNodes;
		}
	}
	message["jobs"] = nlohmann::json::array();
	for (const auto& job: modifiedJobs) {
		message["jobs"].push_back(job->toJson());
	}
	message["nodes"] = nlohmann::json::array();
	for (const auto& node: nodes) {
		message["nodes"].push_back(node->toJson());
	}
	if (forwardIoInformation) {
		message["pfs_read_bw"] = PlatformManager::getPfsReadBandwidth();
		message["pfs_write_bw"] = PlatformManager::getPfsWriteBandwidth();
		message["pfs_read_utilization"] = PlatformManager::getPfsReadUtilization();
		message["pfs_write_utilization"] = PlatformManager::getPfsWriteUtilization();
	}
	PlatformManager::clearModifiedComputeNodes();
	socket.send(zmq::buffer(message.dump()));
}

void SchedulingInterface::init() {
	socket = zmq::socket_t(context, zmq::socket_type::pair);
	socket.bind(Configuration::get("zmq_url"));
}

std::vector<Job*>
SchedulingInterface::handleSchedule(const nlohmann::json& jsonJobs, const std::vector<Job*>& jobQueue) {
	const std::vector<Node*>& nodes = PlatformManager::getComputeNodes();
	std::vector<Job*> scheduledJobs;
	for (const auto& jsonJob: jsonJobs) {
		Job* job = jobQueue[jsonJob["id"]];
		if (jsonJob["kill_flag"]) {
			job->setState(PENDING_KILL);
		} else {
			job->clearAssignedNodes();
			for (const auto& jsonNodeId: jsonJob["assigned_node_ids"]) {
				job->assignNode(nodes[jsonNodeId]);
			}
			if (job->getType() != RIGID) {
				job->assignNumGpusPerNode(jsonJob["assigned_num_gpus_per_node"]);
			}
			if (jsonJob["modified_runtime_args"]) {
				job->clearRuntimeArguments();
				for (const auto& mapping: jsonJob["runtime_arguments"].items()) {
					job->updateRuntimeArguments(mapping.key(), mapping.value());
				}
			}
			job->checkConfigurationValidity();
			job->updateState();
		}
		scheduledJobs.push_back(job);
	}
	return scheduledJobs;
}

std::vector<Job*> SchedulingInterface::schedule(InvocationType invocationType, const std::vector<Job*>& jobQueue,
												const std::vector<Job*>& modifiedJobs, const Job* requestingJob,
												int numberOfNodes) {

	zmq::message_t message;
	nlohmann::json json;

	invokeScheduling(invocationType, modifiedJobs, requestingJob, numberOfNodes);

	std::optional<size_t> result = socket.recv(message, zmq::recv_flags::none);
	if (!result) {
		xbt_die("ZeroMQ communication failed");
	}
	json = nlohmann::json::parse(message.to_string());
	if (json["code"] == ZMQ_SCHEDULED) {
		return handleSchedule(json["jobs"], jobQueue);
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
