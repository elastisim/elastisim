/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "Scheduler.h"

#include <simgrid/s4u.hpp>

#include "Node.h"
#include "WalltimeMonitor.h"
#include "PeriodicInvoker.h"
#include "SimMsg.h"
#include "SchedMsg.h"
#include "NodeMsg.h"
#include "Configuration.h"
#include "SchedulingInterface.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(Scheduler, "Messages within the Scheduler actor");

Scheduler::Scheduler(s4u_Host* masterHost) :
		masterHost(masterHost),
		schedulingInterval(Configuration::exists("scheduling_interval") ?
						   (double) Configuration::get("scheduling_interval") : 0),
		minSchedulingInterval(Configuration::exists("min_scheduling_interval") ?
							  (double) Configuration::get("min_scheduling_interval") : 0), lastInvocation(0),
		scheduleOnJobSubmit(Configuration::getBoolIfExists("schedule_on_job_submit")),
		scheduleOnJobFinalize(Configuration::getBoolIfExists("schedule_on_job_finalize")),
		scheduleOnSchedulingPoint(Configuration::getBoolIfExists("schedule_on_scheduling_point")), currentJobId(0) {
	checkConfigurationValidity();
}

void Scheduler::schedule(InvocationType invocationType, Job* requestingJob) {
	double clock = simgrid::s4u::Engine::get_clock();
	if (minSchedulingInterval == 0 || clock - lastInvocation >= minSchedulingInterval - EPSILON) {
		std::vector<Job*> scheduledJobs = SchedulingInterface::schedule(invocationType, jobQueue, modifiedJobs, requestingJob);
		modifiedJobs.clear();
		if (invocationType == INVOKE_SCHEDULING_POINT) {
			if (requestingJob->getState() == PENDING_RECONFIGURATION) {
				handleReconfiguration(requestingJob);
			} else {
				// continue without reconfiguration
				s4u_Mailbox* mailboxNode;
				for (auto& node: requestingJob->getExecutingNodes()) {
					assignedNodes[requestingJob].insert(node);
					mailboxNode = s4u_Mailbox::by_name(node->getHostName());
					mailboxNode->put(new NodeMsg(NODE_CONTINUE, requestingJob), 0);
				}
			}
		}
		for (auto& job: scheduledJobs) {
			if (job->getState() == PENDING_ALLOCATION) {
				forwardJobAllocation(job);
			} else if (job->getState() == PENDING_KILL) {
				forwardJobKill(job, false);
			}
			modifiedJobs.push_back(job);
		}
		lastInvocation = clock;
	}
}

void Scheduler::handleJobSubmit(Job* job) {
	job->setId(currentJobId++);
	job->setState(PENDING);
	modifiedJobs.push_back(job);
	jobQueue.push_back(job);
	if (scheduleOnJobSubmit) {
		schedule(INVOKE_JOB_SUBMIT, job);
	}
}

void Scheduler::handleProcessedWorkload(Job* job, Node* node) {
	assignedNodes[job].erase(node);
	if (assignedNodes[job].empty()) {
		s4u_Mailbox* mailboxNode;
		for (auto& node: job->getExecutingNodes()) {
			mailboxNode = s4u_Mailbox::by_name(node->getHostName());
			mailboxNode->put(new NodeMsg(NODE_DEALLOCATE, job), 0);
		}
		job->completeWorkload();
		job->setState(COMPLETED);
		modifiedJobs.push_back(job);
		if (job->getWalltime() > 0) {
			walltimeMonitors[job]->kill();
			walltimeMonitors.erase(job);
		}
		s4u_Mailbox* mailboxSimulator = s4u_Mailbox::by_name("SimulationEngine");
		mailboxSimulator->put(new SimMsg(JOB_COMPLETED, job->getId()), 0);
		if (scheduleOnJobFinalize) {
			schedule(INVOKE_JOB_COMPLETED, job);
		}
	}
}

void Scheduler::forwardJobKill(Job* job, bool exceededWalltime) {
	walltimeMonitors.erase(job);
	s4u_Mailbox* mailboxNode;
	for (auto& node: job->getExecutingNodes()) {
		mailboxNode = s4u_Mailbox::by_name(node->getHostName());
		mailboxNode->put(new NodeMsg(NODE_KILL, job), 0);
	}
	job->setState(KILLED);
	modifiedJobs.push_back(job);
	s4u_Mailbox* mailboxSimulator = s4u_Mailbox::by_name("SimulationEngine");
	mailboxSimulator->put(new SimMsg(JOB_KILLED, job->getId()), 0);
	if (exceededWalltime && scheduleOnJobFinalize) {
		schedule(INVOKE_JOB_KILLED, job);
	}
}

void Scheduler::forwardJobAllocation(Job* job) {
	s4u_Mailbox* mailboxNode;
	int rank = 0;
	job->setState(RUNNING);
	modifiedJobs.push_back(job);
	simgrid::s4u::BarrierPtr barrier = s4u_Barrier::create(job->getNumberOfExecutingNodes());
	for (auto& node: job->getExecutingNodes()) {
		assignedNodes[job].insert(node);
		mailboxNode = s4u_Mailbox::by_name(node->getHostName());
		mailboxNode->put(new NodeMsg(NODE_ALLOCATE, job, rank++, barrier), 0);
	}
	if (job->getWalltime() > 0) {
		walltimeMonitors[job] = s4u_Actor::create("WalltimeMonitor@Job" + std::to_string(job->getId()),
												  masterHost, WalltimeMonitor(job));
	}
}

void Scheduler::handleReconfiguration(Job* job) {
	s4u_Mailbox* mailboxNode;
	// continue with reconfiguration
	std::set<Node*> previousNodes(std::begin(job->getExecutingNodes()),
								  std::end(job->getExecutingNodes()));

	// setting the state implies taking over new nodes
	job->setState(IN_RECONFIGURATION);
	std::set<Node*> newNodes(std::begin(job->getExecutingNodes()),
							 std::end(job->getExecutingNodes()));

	int rank = 0;
	std::map<std::string, int> ranks;
	std::vector<Node*> expandNodes;
	simgrid::s4u::BarrierPtr barrier = s4u_Barrier::create(job->getExecutingNodes().size());

	// reconfigure retained nodes or accumulate new nodes to expand
	for (auto& node: job->getExecutingNodes()) {
		assignedNodes[job].insert(node);
		if (previousNodes.find(node) != previousNodes.end()) {
			mailboxNode = s4u_Mailbox::by_name(node->getHostName());
			mailboxNode->put(new NodeMsg(NODE_RECONFIGURE, job, rank++, barrier), 0);
		} else {
			expandNodes.push_back(node);
			ranks[node->getHostName()] = rank++;
		}
	}

	// set and inform expanding nodes for eventual initialization tasks
	int expandRank = 0;
	job->setExpandNodes(expandNodes);
	simgrid::s4u::BarrierPtr expandBarrier = s4u_Barrier::create(expandNodes.size());
	for (auto& node: expandNodes) {
		mailboxNode = s4u_Mailbox::by_name(node->getHostName());
		mailboxNode->put(new NodeMsg(NODE_EXPAND, job, ranks[node->getHostName()],
									 expandRank++, barrier, expandBarrier), 0);
	}

	// deallocate nodes which are no longer assigned in this configuration
	for (auto& node: previousNodes) {
		mailboxNode = s4u_Mailbox::by_name(node->getHostName());
		if (newNodes.find(node) == newNodes.end()) {
			mailboxNode->put(new NodeMsg(NODE_DEALLOCATE, job), 0);
		}
	}
}

void Scheduler::handleSchedulingPoint(Job* job, Node* node, int completedPhases, int remainingIterations) {
	assignedNodes[job].erase(node);
	if (assignedNodes[job].empty()) {
		job->advanceWorkload(completedPhases, remainingIterations);
		modifiedJobs.push_back(job);
		if (scheduleOnSchedulingPoint) {
			schedule(INVOKE_SCHEDULING_POINT, job);
		} else {
			if (job->getState() == PENDING_RECONFIGURATION) {
				handleReconfiguration(job);
			} else {
				// continue without reconfiguration
				s4u_Mailbox* mailboxNode;
				for (auto& node: job->getExecutingNodes()) {
					assignedNodes[job].insert(node);
					mailboxNode = s4u_Mailbox::by_name(node->getHostName());
					mailboxNode->put(new NodeMsg(NODE_CONTINUE, job), 0);
				}
			}
		}
	}
}

void Scheduler::checkConfigurationValidity() const {
	if (schedulingInterval < 0) {
		xbt_die("Scheduling interval can not be less than 0");
	}
	if (minSchedulingInterval < 0) {
		xbt_die("Minimum scheduling interval can not be less than 0");
	}
	if (schedulingInterval == 0 && (!scheduleOnJobSubmit || !scheduleOnJobFinalize)) {
		xbt_die("Scheduling algorithm must be invoked at least periodically or on job submission and job finalization");
	}
}

void Scheduler::operator()() {

	// initialization
	if (schedulingInterval > 0) {
		s4u_Actor::create("PeriodicInvoker", masterHost, PeriodicInvoker(schedulingInterval));
	}
	s4u_Mailbox* mailboxScheduler = s4u_Mailbox::by_name("Scheduler");

	SchedulingInterface::init();

	// main loop
	while (true) {
		auto payload = mailboxScheduler->get_unique<SchedMsg>();
		if (payload->getType() == INVOKE_SCHEDULING) {
			schedule(INVOKE_PERIODIC);
		} else if (payload->getType() == JOB_SUBMIT) {
			XBT_INFO("Received job submission");
			handleJobSubmit(payload->getJob());
		} else if (payload->getType() == SCHEDULING_POINT) {
			XBT_INFO("Received scheduling point from %s running Job %d", payload->getNode()->getHostName().c_str(),
					 payload->getJob()->getId());
			handleSchedulingPoint(payload->getJob(), payload->getNode(), payload->getCompletedPhases(),
								  payload->getRemainingIterations());
		} else if (payload->getType() == WALLTIME_EXCEEDED) {
			XBT_INFO("Received exceeded walltime");
			forwardJobKill(payload->getJob(), true);
		} else if (payload->getType() == WORKLOAD_PROCESSED) {
			XBT_INFO("Received workload processed message from %s running Job %d",
					 payload->getNode()->getHostName().c_str(), payload->getJob()->getId());
			handleProcessedWorkload(payload->getJob(), payload->getNode());
		} else if (payload->getType() == SCHEDULER_FINALIZE) {
			XBT_INFO("Received finalization");
			SchedulingInterface::finalize();
			break;
		}
	}

	// finalization

}
