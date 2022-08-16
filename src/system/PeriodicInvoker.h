/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_PERIODICINVOKER_H
#define ELASTISIM_PERIODICINVOKER_H


class PeriodicInvoker {

private:
	double schedulingInterval;

public:
	explicit PeriodicInvoker(double schedulingInterval);

	void operator()() const;

};


#endif //ELASTISIM_PERIODICINVOKER_H
