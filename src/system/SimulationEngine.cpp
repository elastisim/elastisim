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

#include <indicators.hpp>

#include "Workload.h"
#include "Phase.h"
#include "Utility.h"
#include "PlatformManager.h"
#include "Node.h"
#include "SimMsg.h"
#include "SchedMsg.h"
#include "Configuration.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(SimulationEngine, "Messages within the SimulationEngine actor");

SimulationEngine::SimulationEngine() = default;

void SimulationEngine::operator()() {

	// initialization
	s4u_Mailbox* mailboxSimulator = s4u_Mailbox::by_name("SimulationEngine");
	s4u_Mailbox* mailboxScheduler = s4u_Mailbox::by_name("Scheduler");

	std::ofstream jobStatistics(Configuration::get("job_statistics"));

	const auto& numJobsMsg = mailboxSimulator->get_unique<SimMsg>();
	size_t expectedJobs = numJobsMsg->getNumberOfJobs();
	std::vector<std::unique_ptr<Job>> jobs;
	jobs.reserve(expectedJobs);

	indicators::BlockProgressBar progressBar{
			indicators::option::BarWidth{80},
			indicators::option::ForegroundColor{indicators::Color::green},
			indicators::option::ShowElapsedTime{true},
			indicators::option::ShowRemainingTime{true},
			indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}},
			indicators::option::MaxProgress{expectedJobs},
			indicators::option::PostfixText{"0/" + std::to_string(expectedJobs) + " jobs processed"}
	};

	const bool showProgressBar = (!Configuration::exists("show_progress_bar") ||
			(bool) Configuration::get("show_progress_bar")) && !XBT_LOG_ISENABLED(root, xbt_log_priority_info);

	if (showProgressBar) {
		indicators::show_console_cursor(false);
		progressBar.set_progress(0);
	}

	// main loop
	int processedJobs = 0;
	size_t numberOfJobs = expectedJobs;
	while (expectedJobs > 0) {
		const auto& payload = mailboxSimulator->get_unique<SimMsg>();
		if (payload->getType() == SUBMIT_JOB) {
			XBT_INFO("Registered job submission");
			jobs.push_back(payload->getJob());
			mailboxScheduler->put(new SchedMsg(JOB_SUBMIT, jobs.back().get()), 0);
		} else if (payload->getType() == JOB_COMPLETED) {
			XBT_INFO("Registered job completion");
			expectedJobs--;
			if (showProgressBar) {
				progressBar.set_option(indicators::option::PostfixText{
						std::to_string(++processedJobs) + "/" + std::to_string(numberOfJobs) + " jobs processed"});
				progressBar.tick();
			}
		} else if (payload->getType() == JOB_KILLED) {
			XBT_INFO("Registered job kill");
			expectedJobs--;
			if (showProgressBar) {
				progressBar.set_option(indicators::option::PostfixText{
						std::to_string(++processedJobs) + "/" + std::to_string(numberOfJobs) + " jobs processed"});
				progressBar.tick();
			}
		}
	}

	if (showProgressBar) {
		progressBar.mark_as_completed();
		indicators::show_console_cursor(true);
	}

	// finalization
	XBT_INFO("Send finalization");
	mailboxScheduler->put(new SchedMsg(SCHEDULER_FINALIZE), 0);

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
			case EVOLVING:
				jobStatistics << "evolving" << ",";
				break;
			case ADAPTIVE:
				jobStatistics << "adaptive" << ",";
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
