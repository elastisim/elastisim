/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_GPU_H
#define ELASTISIM_GPU_H

#include <simgrid/s4u.hpp>
#include <json.hpp>

enum GpuState {
	GPU_FREE = 0,
	GPU_ALLOCATED = 1
};

class Gpu {

private:
	int id;
	GpuState state;
	long processingSpeed;
	s4u_Host* host;
	double utilization;
	simgrid::s4u::MutexPtr mutex;
	int kernelId;

	void allocate();

	void deallocate();

public:
	Gpu(int id, long processingSpeed, s4u_Host* host);

	long getProcessingSpeed() const;

	GpuState getState() const;

	double getUtilization() const;

	void exec(double flops);

	s4u_Mailbox* execAsync(double flops);

	nlohmann::json toJson();
};


#endif //ELASTISIM_GPU_H
