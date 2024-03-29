/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_PHASE_H
#define ELASTISIM_PHASE_H


#include <deque>
#include <memory>
#include <map>
#include <string>

class Task;

class Phase {

private:
	std::deque<std::unique_ptr<Task>> tasks;
	std::deque<const Task*> taskPointers;
	int iterations;
	int initialIterations;
	const bool schedulingPoint;
	const std::string evolvingModel;
	const bool barrier;

public:
	Phase(std::deque<std::unique_ptr<Task>> tasks, int iterations, bool schedulingPoint, std::string evolvingModel,
		  bool barrier);

	[[nodiscard]] const std::deque<const Task*>& getTasks() const;

	[[nodiscard]] int getIterations() const;

	void setIterations(int iterations);

	[[nodiscard]] int getInitialIterations() const;

	[[nodiscard]] bool hasSchedulingPoint() const;

	[[nodiscard]] const std::string& getEvolvingModel() const;

	[[nodiscard]] bool hasEvolvingRequest() const;

	[[nodiscard]] bool hasBarrier() const;

	void scaleTo(int numNodes, int numGpusPerNode, const std::map<std::string, std::string>& runtimeArguments);

};


#endif //ELASTISIM_PHASE_H
