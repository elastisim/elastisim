/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_SENSING_H
#define ELASTISIM_SENSING_H


#include <string>

class Sensing {

private:
	const double sensingInterval;

public:
	Sensing();

	void operator()() const;

};


#endif //ELASTISIM_SENSING_H
