/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "SimulationEngine.h"

#include <simgrid/s4u.hpp>
#include <fstream>
#include <memory>

#include "Workload.h"
#include "Phase.h"
#include "Utility.h"
#include "PlatformManager.h"
#include "Node.h"
#include "SimMsg.h"
#include "SchedMsg.h"
#include "NodeMsg.h"
#include "Configuration.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(SimulationEngine, "Messages within the Simulator actor");

SimulationEngine::SimulationEngine() = default;

void SimulationEngine::operator()() {

	// initialization
	s4u_Mailbox* mailboxSimulator = s4u_Mailbox::by_name("SimulationEngine");
	s4u_Mailbox* mailboxScheduler = s4u_Mailbox::by_name("Scheduler");
	s4u_Mailbox* mailboxNode;

	std::ofstream jobStatistics(Configuration::get("job_statistics"));

	const auto& numJobsMsg = mailboxSimulator->get_unique<SimMsg>();
	std::vector<std::unique_ptr<Job>> jobs;
	std::list<int> expectedJobs(numJobsMsg->getNumberOfJobs());
	std::iota(std::begin(expectedJobs), std::end(expectedJobs), 0);

	// main loop
	while (!expectedJobs.empty()) {
		const auto& payload = mailboxSimulator->get_unique<SimMsg>();
		if (payload->getType() == SUBMIT_JOB) {
			XBT_INFO("Registered job submission");
			jobs.push_back(payload->getJob());
			mailboxScheduler->put(new SchedMsg(JOB_SUBMIT, jobs.back().get()), 0);
		} else if (payload->getType() == JOB_COMPLETED) {
			XBT_INFO("Registered job completion");
			expectedJobs.remove(payload->getJobId());
		} else if (payload->getType() == JOB_KILLED) {
			XBT_INFO("Registered job kill");
			expectedJobs.remove(payload->getJobId());
		}
	}

	// finalization
	XBT_INFO("Send finalization");
	mailboxScheduler->put(new SchedMsg(SCHEDULER_FINALIZE), 0);
	for (const auto& node: PlatformManager::getComputeNodes()) {
		mailboxNode = s4u_Mailbox::by_name(node->getHostName());
		mailboxNode->put(new NodeMsg(NODE_FINALIZE), 0);
	}

	jobStatistics << "ID,Type,Submit Time,Start Time,End Time,Wait Time,Makespan,Turnaround Time,Status" << std::endl;
	for (const auto& job: jobs) {
		jobStatistics << job->getId() << ",";
		switch (job->getType()) {
			case RIGID:
				jobStatistics << "rigid" << ",";
				break;
			case MOLDABLE:
				jobStatistics << "moldable" << ",";
				break;
			case MALLEABLE:
				jobStatistics << "malleable" << ",";
				break;
		}
		jobStatistics << job->getSubmitTime() << ",";
		jobStatistics << job->getStartTime() << ",";
		jobStatistics << job->getEndTime() << ",";
		jobStatistics << job->getWaitTime() << ",";
		jobStatistics << job->getMakespan() << ",";
		jobStatistics << job->getTurnaroundTime() << ",";
		if (job->getState() == COMPLETED) {
			jobStatistics << "completed" << std::endl;
		} else if (job->getState() == KILLED) {
			jobStatistics << "killed" << std::endl;
		} else {
			xbt_die("Invalid final job status");
		}
	}

}
