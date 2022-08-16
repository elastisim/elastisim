/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "AsyncSleep.h"

#include <simgrid/s4u.hpp>
#include <utility>

AsyncSleep::AsyncSleep(double duration, std::function<void()> init, std::function<void()> finalize,
					   s4u_Mailbox* callback, void* message) :
		duration(duration), init(std::move(init)), finalize(std::move(finalize)), callback(callback),
		message(message) {}

void AsyncSleep::operator()() const {
	init();
	simgrid::s4u::this_actor::sleep_for(duration);
	finalize();
	if (callback) {
		callback->put(message, 0);
	}
}
