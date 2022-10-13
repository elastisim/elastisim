/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "Gpu.h"
#include "AsyncSleep.h"

Gpu::Gpu(int id, long processingSpeed, s4u_Host* host) :
		id(id), state(GPU_FREE), processingSpeed(processingSpeed), host(host), utilization(0.0),
		mutex(s4u_Mutex::create()), kernelId(0) {}

void Gpu::allocate() {
	state = GPU_ALLOCATED;
	utilization = 1.0;
	mutex->lock();
}

void Gpu::deallocate() {
	mutex->unlock();
	utilization = 0.0;
	state = GPU_FREE;
}

long Gpu::getProcessingSpeed() const {
	return processingSpeed;
}

GpuState Gpu::getState() const {
	return state;
}

double Gpu::getUtilization() const {
	return utilization;
}

void Gpu::exec(double flops) {
	allocate();
	simgrid::s4u::this_actor::sleep_for(flops / processingSpeed);
	deallocate();
}

s4u_Mailbox* Gpu::execAsync(double flops) {
	s4u_Mailbox* callback = s4u_Mailbox::by_name(
			"Kernel" + std::to_string(kernelId++) + "@GPU" + std::to_string(id) + "@" + host->get_name());
	s4u_Actor::create("GPU" + std::to_string(id) + "@" + host->get_name(), host,
					  AsyncSleep(flops / processingSpeed, [this]() { allocate(); }, [this]() { deallocate(); },
								 callback, callback));
	return callback;
}

nlohmann::json Gpu::toJson() {
	nlohmann::json json;
	json["id"] = id;
	json["state"] = state;
	return json;
}
