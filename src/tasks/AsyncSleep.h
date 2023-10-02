/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_ASYNCSLEEP_H
#define ELASTISIM_ASYNCSLEEP_H


#include <memory>
#include <simgrid/s4u.hpp>

class AsyncSleep {

private:
	const double duration;
	std::function<void()> init;
	std::function<void()> finalize;
	s4u_Mailbox* callback;
	void* message;

public:
	AsyncSleep(double duration, std::function<void()> init, std::function<void()> finalize, s4u_Mailbox* callback,
			   void* message);

	void operator()() const;
};


#endif //ELASTISIM_ASYNCSLEEP_H
