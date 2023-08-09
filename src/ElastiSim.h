/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_ELASTISIM_H
#define ELASTISIM_ELASTISIM_H


#include <string>

class ElastiSim {

private:
	[[nodiscard]] static std::string getPropertyIfExists(const char* property);

public:
	static void startSimulation(int host, char* argv[]);

};


#endif //ELASTISIM_ELASTISIM_H
