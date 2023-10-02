/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_SCHEDMSG_H
#define ELASTISIM_SCHEDMSG_H


class Job;

class Node;

enum SchedEventType {
	INVOKE_SCHEDULING,
	JOB_SUBMIT,
	WALLTIME_EXCEEDED,
	SCHEDULING_POINT,
	WORKLOAD_PROCESSED,
	SCHEDULER_FINALIZE
};

class SchedMsg {

private:
	const SchedEventType type;
	Job* job;

public:

	explicit SchedMsg(SchedEventType type);

	SchedMsg(SchedEventType type, Job* job);

	[[nodiscard]] SchedEventType getType() const;

	[[nodiscard]] Job* getJob() const;

};


#endif //ELASTISIM_SCHEDMSG_H
