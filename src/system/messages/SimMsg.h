/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_SIMMSG_H
#define ELASTISIM_SIMMSG_H

#include <memory>

class Job;

enum SimEventType {
	NUMBER_OF_JOBS,
	SUBMIT_JOB,
	JOB_COMPLETED,
	JOB_KILLED
};

class SimMsg {

private:
	const SimEventType type;
	const size_t numberOfJobs;
	std::unique_ptr<Job> job;

public:
	SimMsg(SimEventType type, size_t numberOfJobs);

	SimMsg(SimEventType type, std::unique_ptr<Job> job);

	[[nodiscard]] SimEventType getType() const;

	[[nodiscard]] size_t getNumberOfJobs() const;

	[[nodiscard]] std::unique_ptr<Job> getJob();

};


#endif //ELASTISIM_SIMMSG_H
