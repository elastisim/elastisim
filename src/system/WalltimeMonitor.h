/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_WALLTIMEMONITOR_H
#define ELASTISIM_WALLTIMEMONITOR_H

class Job;

class WalltimeMonitor {

private:
	Job* job;
	const double gracePeriod;

public:
	explicit WalltimeMonitor(Job* job, double gracePeriod);

	void operator()();
};


#endif //ELASTISIM_WALLTIMEMONITOR_H
