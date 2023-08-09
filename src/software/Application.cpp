/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "Application.h"

#include "Job.h"
#include "Phase.h"
#include "Workload.h"
#include "Node.h"
#include "Task.h"
#include "NodeMsg.h"
#include "Utility.h"


XBT_LOG_NEW_DEFAULT_CATEGORY(Application, "Messages within the application");

Application::Application(Node* node, Job* job, int rank) : node(node), job(job), rank(rank) {}

void Application::waitForAsyncActivities(const std::vector<simgrid::s4u::ActivityPtr>& asyncActivities) {
	for (const auto& activity: asyncActivities) {
		activity->wait();
	}
}

void Application::executeOneTimePhase(const Phase* phase, const Node* node,
									  const Job* job, const std::vector<Node*>& nodes,
									  int rank, const simgrid::s4u::BarrierPtr& barrier) {

	if (phase == nullptr) {
		return;
	}
	std::vector<simgrid::s4u::ActivityPtr> asyncActivities;
	for (int i = 0; i < phase->getIterations(); ++i) {
		for (const auto& task: phase->getTasks()) {
			int iterations = task->getIterations();
			double taskStart = Utility::logTaskStart(task, iterations);
			for (int j = 0; j < iterations; ++j) {
				double iterationStart = Utility::logIterationStart(iterations, j);
				if (task->isSynchronized()) {
					barrier->wait();
				}
				if (task->isAsynchronous()) {
					std::vector<simgrid::s4u::ActivityPtr> activities = task->executeAsync(node, job, nodes, rank);
					asyncActivities.insert(std::end(asyncActivities), std::begin(activities), std::end(activities));
				} else {
					task->execute(node, job, nodes, rank, barrier);
				}
				Utility::logIterationEnd(iterations, j, iterationStart);
			}
			node->logTaskTime(job, task, Utility::logTaskEnd(task, taskStart));
		}
		if (phase->hasBarrier()) {
			barrier->wait();
		}
	}
	for (const auto& activity: asyncActivities) {
		activity->wait();
	}
}

void Application::operator()() {

	if (node->isInitializing(job)) {
		executeOneTimePhase(job->getWorkload()->getInitPhase(), node, job, job->getExecutingNodes(), rank,
							node->getBarrier(job));
		node->markInitialized(job);
	}

	if (node->isReconfiguring(job)) {
		executeOneTimePhase(job->getWorkload()->getReconfigurationPhase(), node, job, job->getExecutingNodes(), rank,
							node->getBarrier(job));
		node->markReconfigured(job);
	}

	if (rank == 0) {
		job->setState(RUNNING);
	}

	if (node->isExpanding(job)) {
		executeOneTimePhase(job->getWorkload()->getExpansionPhase(), node, job, job->getExpandingNodes(),
							node->getExpandRank(job), node->getExpandBarrier(job));
		node->markExpanded(job);
	}

	std::deque<const Phase*> phaseQueue = job->getWorkload()->getPhases();
	const Phase* phase = phaseQueue.front();
	int remainingIterations = phase->getIterations();
	int completedPhases = 0;

	const simgrid::s4u::BarrierPtr& barrier = node->getBarrier(job);
	std::vector<simgrid::s4u::ActivityPtr> asyncActivities;

	while (remainingIterations > 0) {
		std::deque<const Task*> taskQueue = phase->getTasks();
		while (!taskQueue.empty()) {
			const Task* task = taskQueue.front();
			int iterations = task->getIterations();
			double taskStart = Utility::logTaskStart(task, iterations);
			for (int i = 0; i < iterations; ++i) {
				double iterationStart = Utility::logIterationStart(iterations, i);
				if (task->isSynchronized()) {
					barrier->wait();
				}
				if (task->isAsynchronous()) {
					std::vector<simgrid::s4u::ActivityPtr> activities =
							task->executeAsync(node, job, job->getExecutingNodes(), rank);
					asyncActivities.insert(std::end(asyncActivities), std::begin(activities), std::end(activities));
				} else {
					task->execute(node, job, job->getExecutingNodes(), rank, barrier);
				}
				Utility::logIterationEnd(iterations, i, iterationStart);
			}
			node->logTaskTime(job, task, Utility::logTaskEnd(task, taskStart));
			taskQueue.pop_front();
		}
		--remainingIterations;
		if (remainingIterations == 0) {
			phaseQueue.pop_front();
			++completedPhases;
		}
		if (phaseQueue.empty()) {
			waitForAsyncActivities(asyncActivities);
			asyncActivities.clear();
			s4u_Mailbox* mailboxNode = s4u_Mailbox::by_name(node->getHostName());
			mailboxNode->put(new NodeMsg(WORKLOAD_COMPLETED, job), 0);
		} else if (job->getType() == MALLEABLE && phase->hasSchedulingPoint() &&
				   !(remainingIterations == 0 && !phase->hasFinalSchedulingPoint())) {
			waitForAsyncActivities(asyncActivities);
			asyncActivities.clear();
			s4u_Mailbox* mailboxNode = s4u_Mailbox::by_name(node->getHostName());
			mailboxNode->put(new NodeMsg(AT_SCHEDULING_POINT, job, completedPhases, remainingIterations),
							 0);
			break;
		} else {
			if (phase->hasBarrier()) {
				barrier->wait();
			}
			if (remainingIterations == 0) {
				phase = phaseQueue.front();
				remainingIterations = phase->getIterations();
			}
		}
	}

}
