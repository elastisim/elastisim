/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_SCHEDULINGINTERFACE_H
#define ELASTISIM_SCHEDULINGINTERFACE_H


#include <zmq.hpp>
#include <json.hpp>

class Job;

class Node;

enum CommunicationCode {
	ZMQ_INVOKE_SCHEDULING = 0xFFEC4400,
	ZMQ_SCHEDULED = 0xFFEC4401,
	ZMQ_FINALIZE = 0xFFEC44FF
};

class SchedulingInterface {

private:
	static zmq::context_t context;
	static zmq::socket_t socket;

	static void invokeScheduling(const std::vector<Job*>& jobQueue);

public:
	static void init();

	static void handleSchedule(const nlohmann::json& jsonJobs, const std::vector<Job*>& jobQueue);

	static void schedule(const std::vector<Job*>& jobQueue);

	static void finalize();

};


#endif //ELASTISIM_SCHEDULINGINTERFACE_H
