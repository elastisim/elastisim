/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "SequenceTask.h"

#include <utility>
#include <xbt.h>
#include "Job.h"
#include "Utility.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(SequenceTask, "Messages within the Sequence Task");

SequenceTask::SequenceTask(const std::string& name, const std::string& iterations, bool synchronized,
						   std::deque<std::unique_ptr<Task>> tasks) :
		Task(name, iterations, synchronized), tasks(std::move(tasks)) {}

double SequenceTask::logTaskStart(const Task* task, int iterations) {
	if (task->getName().empty()) {
		XBT_INFO("Starting task with %d iteration(s)...", iterations);
	} else {
		XBT_INFO("Starting task %s with %d iteration(s)...", task->getName().c_str(), iterations);
	}
	return simgrid::s4u::Engine::get_clock();
}

void SequenceTask::logTaskEnd(const Task* task, double start) {
	if (task->getName().empty()) {
		XBT_INFO("Task finished after %f seconds", simgrid::s4u::Engine::get_clock() - start);
	} else {
		XBT_INFO("Task %s finished after %f seconds", task->getName().c_str(),
				 simgrid::s4u::Engine::get_clock() - start);
	}
}

double SequenceTask::logIterationStart(int iterations, int i) {
	if (iterations > 1) {
		XBT_INFO("Executing iteration %d...", i);
	}
	return simgrid::s4u::Engine::get_clock();
}

void SequenceTask::logIterationEnd(int iterations, int i, double start) {
	if (iterations > 1) {
		XBT_INFO("Finished iteration %d after %f seconds", i, simgrid::s4u::Engine::get_clock() - start);
	}
}

void SequenceTask::execute(const Node* node, const Job* job, const std::vector<Node*>& nodes, int rank,
						   simgrid::s4u::BarrierPtr barrier) const {
	std::vector<simgrid::s4u::ActivityPtr> asyncActivities;
	for (auto& task: tasks) {
		int iterations = task->getIterations();
		double taskStart = logTaskStart(task.get(), iterations);
		for (int i = 0; i < iterations; ++i) {
			double iterationStart = logIterationStart(iterations, i);
			if (task->isSynchronized()) {
				barrier->wait();
			}
			if (task->isAsynchronous()) {
				std::vector<simgrid::s4u::ActivityPtr> activities = task->executeAsync(node, job, nodes, rank);
				asyncActivities.insert(std::end(asyncActivities), std::begin(activities), std::end(activities));
			} else {
				task->execute(node, job, nodes, rank, barrier);
			}
			logIterationEnd(iterations, i, iterationStart);
		}
		logTaskEnd(task.get(), taskStart);
	}
	for (auto& activity: asyncActivities) {
		activity->wait();
	}
}

void SequenceTask::scaleTo(int numNodes, int numGpusPerNode) {
	Task::scaleTo(numNodes, numGpusPerNode);
	for (auto& task: tasks) {
		task->scaleTo(numNodes, numGpusPerNode);
	}
}
