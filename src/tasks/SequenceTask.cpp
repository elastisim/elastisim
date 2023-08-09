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
#include "Node.h"
#include "Utility.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(SequenceTask, "Messages within the Sequence Task");

SequenceTask::SequenceTask(const std::string& name, const std::string& iterations, bool synchronized,
						   std::deque<std::unique_ptr<Task>> tasks) :
		Task(name, iterations, synchronized), tasks(std::move(tasks)) {}

void SequenceTask::execute(const Node* node, const Job* job, const std::vector<Node*>& nodes, int rank,
						   simgrid::s4u::BarrierPtr barrier) const {
	std::vector<simgrid::s4u::ActivityPtr> asyncActivities;
	for (const auto& task: tasks) {
		int iterations = task->getIterations();
		double taskStart = Utility::logTaskStart(task.get(), iterations);
		for (int i = 0; i < iterations; ++i) {
			double iterationStart = Utility::logIterationStart(iterations, i);
			if (task->isSynchronized()) {
				barrier->wait();
			}
			if (task->isAsynchronous()) {
				std::vector<simgrid::s4u::ActivityPtr> activities = task->executeAsync(node, job, nodes, rank);
				asyncActivities.insert(std::end(asyncActivities), std::begin(activities), std::end(activities));
			} else {
				task->execute(node, job, nodes, rank, barrier);
			}
			Utility::logIterationEnd(iterations, i, iterationStart);
		}
		node->logTaskTime(job, task.get(), Utility::logTaskEnd(task.get(), taskStart));
	}
	for (const auto& activity: asyncActivities) {
		activity->wait();
	}
}

void
SequenceTask::scaleTo(int numNodes, int numGpusPerNode, const std::map<std::string, std::string>& runtimeArguments) {
	Task::scaleTo(numNodes, numGpusPerNode, runtimeArguments);
	for (const auto& task: tasks) {
		task->scaleTo(numNodes, numGpusPerNode, runtimeArguments);
	}
}
