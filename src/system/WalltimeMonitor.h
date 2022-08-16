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

#include <memory>

class Job;

class WalltimeMonitor {

private:
	Job* job;

public:
	explicit WalltimeMonitor(Job* job);

	void operator()();
};


#endif //ELASTISIM_WALLTIMEMONITOR_H
