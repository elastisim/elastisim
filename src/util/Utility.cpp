/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include "Utility.h"

#include <simgrid/s4u.hpp>
#include <regex>

#include <exprtk.hpp>

#include "Phase.h"
#include "Workload.h"
#include "BusyWaitTask.h"
#include "PfsReadTask.h"
#include "BurstBufferReadTask.h"
#include "BurstBufferWriteTask.h"
#include "PfsWriteTask.h"
#include "SequenceTask.h"
#include "IdleTask.h"
#include "CombinedCpuTask.h"
#include "CombinedGpuTask.h"

#define EUCLIDIAN_MOD(a, b)  ((a) < 0 ? ((((a) % (b)) + (b)) % (b)) : ((a) % (b)))

XBT_LOG_NEW_DEFAULT_CATEGORY(Utility, "Messages within Utility");

std::string Utility::toLower(const std::string& string) {
	std::string lowerString(string);
	std::transform(std::begin(string), std::end(string), std::begin(lowerString),
				   [](const char c) { return std::tolower(c); });
	return lowerString;
}

JobType Utility::parseJobType(const std::string& jobType) {
	if (toLower(jobType) == "rigid") {
		return RIGID;
	} else if (toLower(jobType) == "moldable") {
		return MOLDABLE;
	} else if (toLower(jobType) == "malleable") {
		return MALLEABLE;
	} else {
		xbt_die("Unknown job type %s", jobType.c_str());
	}
}

std::string Utility::asString(VectorPattern pattern) {
	switch (pattern) {
		case ALL_RANKS:
			return "ALL_RANKS";
		case ROOT_ONLY:
			return "ROOT_ONLY";
		case EVEN_RANKS:
			return "EVEN_RANKS";
		case ODD_RANKS:
			return "ODD_RANKS";
		case VECTOR:
			return "VECTOR";
		case UNIFORM:
			return "UNIFORM";
		default:
			xbt_die("Unknown vector pattern");
	}
}

VectorPattern Utility::asVectorPattern(const std::string& pattern) {
	if (toLower(pattern) == "root_only") {
		return ROOT_ONLY;
	} else if (toLower(pattern) == "all_ranks" || toLower(pattern) == "total") {
		return ALL_RANKS;
	} else if (toLower(pattern) == "even_ranks") {
		return EVEN_RANKS;
	} else if (toLower(pattern) == "odd_ranks") {
		return ODD_RANKS;
	} else if (toLower(pattern) == "uniform") {
		return UNIFORM;
	} else if (toLower(pattern) == "vector") {
		return VECTOR;
	} else {
		xbt_die("Unknown vector pattern type %s", pattern.c_str());
	}
}

std::string Utility::asString(MatrixPattern pattern) {
	switch (pattern) {
		case ALL_TO_ALL:
			return "ALL_TO_ALL";
		case GATHER:
			return "GATHER";
		case SCATTER:
			return "SCATTER";
		case MASTER_WORKER:
			return "MASTER_WORKER";
		case RING:
			return "RING";
		case RING_CLOCKWISE:
			return "RING_CLOCKWISE";
		case RING_COUNTER_CLOCKWISE:
			return "RING_COUNTER_CLOCKWISE";
		case MATRIX:
			return "MATRIX";
		default:
			xbt_die("Unknown matrix pattern");
	}
}

MatrixPattern Utility::asMatrixPattern(const std::string& pattern) {
	if (toLower(pattern) == "all_to_all") {
		return ALL_TO_ALL;
	} else if (toLower(pattern) == "gather") {
		return GATHER;
	} else if (toLower(pattern) == "scatter") {
		return SCATTER;
	} else if (toLower(pattern) == "master_worker") {
		return MASTER_WORKER;
	} else if (toLower(pattern) == "ring") {
		return RING;
	} else if (toLower(pattern) == "ring_clockwise") {
		return RING_CLOCKWISE;
	} else if (toLower(pattern) == "ring_counter_clockwise") {
		return RING_COUNTER_CLOCKWISE;
	} else if (toLower(pattern) == "matrix") {
		return MATRIX;
	} else {
		xbt_die("Unknown matrix pattern type %s", pattern.c_str());
	}
}

std::string Utility::applyArguments(const std::string& model, const std::map<std::string, std::string>& arguments) {
	std::string substitutedModel = model;
	for (auto& argument: arguments) {
		substitutedModel = std::regex_replace(substitutedModel, std::regex(argument.first), argument.second);
	}
	return substitutedModel;
}

std::map<std::string, std::string> Utility::readStringMap(nlohmann::json jsonMap) {
	std::map<std::string, std::string> map;
	for (auto& mapping: jsonMap.items()) {
		if (mapping.value().is_string()) {
			map[mapping.key()] = mapping.value();
		} else if (mapping.value().is_number_float()) {
			double value = mapping.value();
			map[mapping.key()] = std::to_string(value);
		} else if (mapping.value().is_number_integer()) {
			long value = mapping.value();
			map[mapping.key()] = std::to_string(value);
		} else {
			xbt_die("Invalid type for mapping %s", mapping.key().c_str());
		}
	}
	return map;
}

std::unique_ptr<Task>
Utility::readTask(nlohmann::json jsonTask, const std::map<std::string, std::string>& arguments, int numNodes,
				  int numGpusPerNode) {

	std::string name = "";
	if (jsonTask["name"].is_string()) {
		name = jsonTask["name"];
	}
	std::string iterations;
	if (jsonTask["iterations"].is_string()) {
		iterations = applyArguments(jsonTask["iterations"], arguments);
	} else if (jsonTask["iterations"].is_number_unsigned()) {
		int iterationsInteger = jsonTask["iterations"];
		iterations = std::to_string(iterationsInteger);
	} else {
		iterations = "1";
	}

	bool synchronized = false;
	if (jsonTask["synchronized"].is_boolean()) {
		synchronized = jsonTask["synchronized"];
	}

	std::unique_ptr<Task> task;
	if (toLower(jsonTask["type"]) == "busy_wait") {
		task = createDelayTask<BusyWaitTask>(jsonTask, name, iterations, synchronized, arguments, numNodes,
											 numGpusPerNode);
	} else if (toLower(jsonTask["type"]) == "idle") {
		task = createDelayTask<IdleTask>(jsonTask, name, iterations, synchronized, arguments, numNodes, numGpusPerNode);
	} else if (toLower(jsonTask["type"]) == "cpu") {
		task = createCombinedCpuTask(jsonTask, name, iterations, synchronized, arguments, numNodes, numGpusPerNode);
	} else if (toLower(jsonTask["type"]) == "gpu") {
		task = createCombinedGpuTask(jsonTask, name, iterations, synchronized, arguments, numNodes, numGpusPerNode);
	} else if (toLower(jsonTask["type"]) == "pfs_read") {
		task = createIoTask<PfsReadTask>(jsonTask, name, iterations, synchronized, arguments, numNodes, numGpusPerNode);
	} else if (toLower(jsonTask["type"]) == "pfs_write") {
		task = createIoTask<PfsWriteTask>(jsonTask, name, iterations, synchronized, arguments, numNodes,
										  numGpusPerNode);
	} else if (toLower(jsonTask["type"]) == "bb_read") {
		task = createIoTask<BurstBufferReadTask>(jsonTask, name, iterations, synchronized, arguments, numNodes,
												 numGpusPerNode);
	} else if (toLower(jsonTask["type"]) == "bb_write") {
		task = createIoTask<BurstBufferWriteTask>(jsonTask, name, iterations, synchronized, arguments, numNodes,
												  numGpusPerNode);
	} else if (toLower(jsonTask["type"]) == "sequence") {
		task = createSequenceTask(jsonTask, name, iterations, synchronized, arguments, numNodes, numGpusPerNode);
	} else {
		std::string taskType = jsonTask["type"];
		xbt_die("Invalid task type %s", taskType.c_str());
	}
	if (numNodes > 0) {
		task->updateIterations(numNodes, numGpusPerNode);
	}
	return task;
}

std::unique_ptr<Phase>
Utility::readPhase(nlohmann::json jsonPhase, const std::map<std::string, std::string>& arguments, int numNodes,
				   int numGpusPerNode) {
	int iterations = 1;
	if (jsonPhase["iterations"].is_number_unsigned()) {
		iterations = jsonPhase["iterations"];
	} else if (jsonPhase["iterations"].is_string()) {
		iterations = std::stoi(applyArguments(jsonPhase["iterations"], arguments));
	}
	bool schedulingPoint = true;
	if (jsonPhase["scheduling_point"].is_boolean()) {
		schedulingPoint = jsonPhase["scheduling_point"];
	}
	bool finalSchedulingPoint = true;
	if (jsonPhase["final_scheduling_point"].is_boolean()) {
		finalSchedulingPoint = jsonPhase["final_scheduling_point"];
	}
	bool barrier = true;
	if (jsonPhase["barrier"].is_boolean()) {
		barrier = jsonPhase["barrier"];
	}
	std::deque<std::unique_ptr<Task>> tasks;
	for (auto& task: jsonPhase["tasks"]) {
		tasks.push_back(readTask(task, arguments, numNodes, numGpusPerNode));
	}
	return std::make_unique<Phase>(std::move(tasks), iterations, schedulingPoint, finalSchedulingPoint, barrier);
}

std::unique_ptr<Phase>
Utility::readOneTimePhase(nlohmann::json jsonPhase, const std::map<std::string, std::string>& arguments,
						  bool mandatoryBarrier, int numNodes, int numGpusPerNode) {

	if (jsonPhase.is_null()) {
		return nullptr;
	}
	int iterations = 1;
	if (jsonPhase["iterations"].is_number_unsigned()) {
		iterations = jsonPhase["iterations"];
	}
	bool schedulingPoint = false;
	bool finalSchedulingPoint = false;

	bool barrier;
	if (mandatoryBarrier) {
		barrier = true;
	} else {
		barrier = false;
		if (jsonPhase["barrier"].is_boolean()) {
			barrier = jsonPhase["barrier"];
		}
	}
	std::deque<std::unique_ptr<Task>> tasks;
	for (auto& task: jsonPhase["tasks"]) {
		tasks.push_back(readTask(task, arguments, numNodes, numGpusPerNode));
	}
	return std::make_unique<Phase>(std::move(tasks), iterations, schedulingPoint, finalSchedulingPoint, barrier);
}

std::unique_ptr<Workload>
Utility::readWorkload(const std::string& workloadFile, const std::map<std::string, std::string>& arguments,
					  int numNodes, int numGpusPerNode) {
	std::ifstream stream(workloadFile);
	nlohmann::json json = nlohmann::json::parse(stream);
	std::unique_ptr<Phase> onInitialize = readOneTimePhase(json["on_init"], arguments, false, numNodes, numGpusPerNode);
	std::unique_ptr<Phase> onReconfiguration = readOneTimePhase(json["on_reconfiguration"], arguments, true, numNodes,
																numGpusPerNode);
	std::unique_ptr<Phase> onExpansion = readOneTimePhase(json["on_expansion"], arguments, false, numNodes,
														  numGpusPerNode);
	std::deque<std::unique_ptr<Phase>> phases;
	for (auto& phase: json["phases"]) {
		phases.push_back(readPhase(phase, arguments, numNodes, numGpusPerNode));
	}
	return std::make_unique<Workload>(std::move(onInitialize), std::move(onReconfiguration), std::move(onExpansion),
									  std::move(phases));
}

template<typename T>
std::unique_ptr<T>
Utility::createDelayTask(nlohmann::json jsonTask, const std::string& name, const std::string& iterations,
						 bool synchronized, const std::map<std::string, std::string>& arguments, int numNodes,
						 int numGpusPerNode) {

	VectorPattern pattern = asVectorPattern(jsonTask["pattern"]);
	if (numNodes == 0) {
		if (pattern == VECTOR) {
			xbt_die("Invalid pattern type %s for malleable job", asString(pattern).c_str());
		} else {
			if (jsonTask["delay"].is_number()) {
				std::string delays = std::to_string((double) jsonTask["delay"]);
				return std::make_unique<T>(name, iterations, synchronized, delays, pattern);
			} else if (jsonTask["delay"].is_string()) {
				std::string delays = applyArguments(jsonTask["delay"], arguments);
				return std::make_unique<T>(name, iterations, synchronized, delays, pattern);
			} else {
				xbt_die("%s pattern requires a number or string type", asString(pattern).c_str());
			}
		}
	} else {
		if (pattern == VECTOR) {
			if (jsonTask["delay"].is_primitive()) {
				xbt_die("VECTOR pattern requires an array type");
			}
			std::vector<double> delays = jsonTask["delay"];
			return std::make_unique<T>(name, iterations, synchronized, delays, pattern);
		} else {
			if (jsonTask["delay"].is_number()) {
				std::vector<double> delays = createVector((double) jsonTask["delay"], pattern, numNodes);
				return std::make_unique<T>(name, iterations, synchronized, delays, pattern);
			} else if (jsonTask["delay"].is_string()) {
				std::vector<double> delays = createVector(applyArguments(jsonTask["delay"], arguments), pattern,
														  numNodes, numGpusPerNode);
				return std::make_unique<T>(name, iterations, synchronized, delays, pattern);
			} else {
				xbt_die("%s pattern requires a number or string type", asString(pattern).c_str());
			}
		}
	}
}

template<typename T>
std::unique_ptr<T>
Utility::createIoTask(nlohmann::json jsonTask, const std::string& name, const std::string& iterations,
					  bool synchronized, const std::map<std::string, std::string>& arguments, int numNodes,
					  int numGpusPerNode) {

	bool async = false;
	if (jsonTask["async"].is_boolean()) {
		async = jsonTask["async"];
	}

	VectorPattern pattern = asVectorPattern(jsonTask["pattern"]);
	if (numNodes == 0) {
		if (pattern == VECTOR) {
			xbt_die("Invalid pattern type %s for malleable job", asString(pattern).c_str());
		} else {
			if (jsonTask["bytes"].is_number()) {
				std::string ioSizes = std::to_string((double) jsonTask["bytes"]);
				return std::make_unique<T>(name, iterations, synchronized, async, ioSizes, pattern);
			} else if (jsonTask["bytes"].is_string()) {
				std::string ioSizes = applyArguments(jsonTask["bytes"], arguments);
				return std::make_unique<T>(name, iterations, synchronized, async, ioSizes, pattern);
			} else {
				xbt_die("%s pattern requires a number or string type", asString(pattern).c_str());
			}
		}
	} else {
		if (pattern == VECTOR) {
			if (jsonTask["bytes"].is_primitive()) {
				xbt_die("VECTOR pattern requires an array type");
			}
			std::vector<double> ioSizes = jsonTask["bytes"];
			return std::make_unique<T>(name, iterations, synchronized, async, ioSizes, pattern);
		} else {
			if (jsonTask["bytes"].is_number()) {
				std::vector<double> ioSizes = createVector((double) jsonTask["bytes"], pattern, numNodes);
				return std::make_unique<T>(name, iterations, synchronized, async, ioSizes, pattern);
			} else if (jsonTask["bytes"].is_string()) {
				std::vector<double> ioSizes = createVector(applyArguments(jsonTask["bytes"], arguments), pattern,
														   numNodes, numGpusPerNode);
				return std::make_unique<T>(name, iterations, synchronized, async, ioSizes, pattern);
			} else {
				xbt_die("%s pattern requires a number or string type", asString(pattern).c_str());
			}
		}
	}
}

std::unique_ptr<Task>
Utility::createCombinedGpuTask(nlohmann::json jsonTask, const std::string& name, const std::string& iterations,
							   bool synchronized, const std::map<std::string, std::string>& arguments, int numNodes,
							   int numGpusPerNode) {

	if (jsonTask["flops"].is_null() && jsonTask["bytes"].is_null()) {
		xbt_die("FLOPS and payloads can not be simultaneously unspecified for the same task");
	}

	std::vector<double> flops;
	std::string gpuModel;

	VectorPattern gpuPattern;
	if (!jsonTask["flops"].is_null()) {
		gpuPattern = asVectorPattern(jsonTask["computation_pattern"]);
		if (numNodes == 0) {
			if (gpuPattern == VECTOR) {
				xbt_die("Invalid gpuPattern type %s for malleable job", asString(gpuPattern).c_str());
			} else {
				if (jsonTask["flops"].is_number()) {
					gpuModel = std::to_string((double) jsonTask["flops"]);
				} else if (jsonTask["flops"].is_string()) {
					gpuModel = applyArguments(jsonTask["flops"], arguments);
				} else {
					xbt_die("%s computation_pattern requires a number or string type", asString(gpuPattern).c_str());
				}
			}
		} else {
			if (gpuPattern == VECTOR) {
				if (jsonTask["flops"].is_primitive()) {
					xbt_die("VECTOR computation_pattern requires an array type");
				}
				std::vector<double> local = jsonTask["flops"];
				flops = std::move(local);
			} else {
				if (jsonTask["flops"].is_number()) {
					flops = createVector((double) jsonTask["flops"], gpuPattern, numNodes);
				} else if (jsonTask["flops"].is_string()) {
					flops = createVector(applyArguments(jsonTask["flops"], arguments), gpuPattern, numNodes,
										 numGpusPerNode);
				} else {
					xbt_die("%s computation_pattern requires a number or string type", asString(gpuPattern).c_str());
				}
			}
		}
	}

	std::vector<double> intraNodeCommunication;
	std::vector<double> interNodeCommunication;
	std::string comModel;

	MatrixPattern comPattern;
	if (!jsonTask["bytes"].is_null()) {
		comPattern = asMatrixPattern(jsonTask["communication_pattern"]);
		if (numNodes == 0) {
			if (jsonTask["bytes"].is_number()) {
				comModel = std::to_string((double) jsonTask["bytes"]);
			} else if (jsonTask["bytes"].is_string()) {
				comModel = applyArguments(jsonTask["bytes"], arguments);
			} else {
				xbt_die("Payloads require a number or string type");
			}
		} else {
			if (comPattern == MATRIX) {
				xbt_die("MATRIX communication_pattern not supported for GPU tasks");
			} else {
				if (jsonTask["bytes"].is_number()) {
					std::tie(intraNodeCommunication, interNodeCommunication) =
							createMatrices((double) jsonTask["bytes"], comPattern, numNodes, numGpusPerNode);
				} else if (jsonTask["bytes"].is_string()) {
					std::tie(intraNodeCommunication, interNodeCommunication) =
							createMatrices(applyArguments(jsonTask["bytes"], arguments), comPattern, numNodes,
										   numGpusPerNode);
				} else {
					xbt_die("Payloads require a number or string type");
				}
			}
		}

	}

	if (gpuModel.empty() && comModel.empty()) {
		return std::make_unique<CombinedGpuTask>(name, iterations, synchronized, flops, intraNodeCommunication,
												 interNodeCommunication);
	} else if (gpuModel.empty() && !comModel.empty()) {
		return std::make_unique<CombinedGpuTask>(name, iterations, synchronized, flops, comModel, comPattern);
	} else if (!gpuModel.empty() && comModel.empty()) {
		return std::make_unique<CombinedGpuTask>(name, iterations, synchronized, gpuModel, gpuPattern,
												 intraNodeCommunication,
												 interNodeCommunication);
	} else {
		return std::make_unique<CombinedGpuTask>(name, iterations, synchronized, gpuModel, gpuPattern, comModel,
												 comPattern);
	}

}

std::unique_ptr<Task>
Utility::createCombinedCpuTask(nlohmann::json jsonTask, const std::string& name, const std::string& iterations,
							   bool synchronized, const std::map<std::string, std::string>& arguments, int numNodes,
							   int numGpusPerNode) {

	if (jsonTask["flops"].is_null() && jsonTask["bytes"].is_null()) {
		xbt_die("FLOPS and payloads can not be simultaneously unspecified for the same task");
	}

	std::vector<double> flops;
	std::string cpuModel;

	VectorPattern cpuPattern;
	if (!jsonTask["flops"].is_null()) {
		cpuPattern = asVectorPattern(jsonTask["computation_pattern"]);
		if (numNodes == 0) {
			if (cpuPattern == VECTOR) {
				xbt_die("Invalid cpuPattern type %s for malleable job", asString(cpuPattern).c_str());
			} else {
				if (jsonTask["flops"].is_number()) {
					cpuModel = std::to_string((double) jsonTask["flops"]);
				} else if (jsonTask["flops"].is_string()) {
					cpuModel = applyArguments(jsonTask["flops"], arguments);
				} else {
					xbt_die("%s computation_pattern requires a number or string type", asString(cpuPattern).c_str());
				}
			}
		} else {
			if (cpuPattern == VECTOR) {
				if (jsonTask["flops"].is_primitive()) {
					xbt_die("VECTOR computation_pattern requires an array type");
				}
				std::vector<double> local = jsonTask["flops"];
				flops = std::move(local);
			} else {
				if (jsonTask["flops"].is_number()) {
					flops = createVector((double) jsonTask["flops"], cpuPattern, numNodes);
				} else if (jsonTask["flops"].is_string()) {
					flops = createVector(applyArguments(jsonTask["flops"], arguments), cpuPattern, numNodes,
										 numGpusPerNode);
				} else {
					xbt_die("%s computation_pattern requires a number or string type", asString(cpuPattern).c_str());
				}
			}
		}
	}

	std::vector<double> bytes;
	std::string comModel;

	MatrixPattern comPattern;
	if (!jsonTask["bytes"].is_null()) {
		comPattern = asMatrixPattern(jsonTask["communication_pattern"]);
		if (numNodes == 0) {
			if (jsonTask["bytes"].is_number()) {
				comModel = std::to_string((double) jsonTask["bytes"]);
			} else if (jsonTask["bytes"].is_string()) {
				comModel = applyArguments(jsonTask["bytes"], arguments);
			} else {
				xbt_die("Payloads require a number or string type");
			}
		} else {
			if (comPattern == MATRIX) {
				if (jsonTask["bytes"].is_primitive()) {
					xbt_die("MATRIX communication_pattern requires an array type");
				}
				std::vector<double> local = jsonTask["bytes"];
				bytes = std::move(local);
			} else {
				if (jsonTask["bytes"].is_number()) {
					bytes = createMatrix((double) jsonTask["bytes"], comPattern, numNodes);
				} else if (jsonTask["bytes"].is_string()) {
					bytes = createMatrix(applyArguments(jsonTask["bytes"], arguments), comPattern, numNodes,
										 numGpusPerNode);
				} else {
					xbt_die("Payloads require a number or string type");
				}
			}
		}

	}

	bool coupled = false;
	if (jsonTask["coupled"].is_boolean()) {
		coupled = jsonTask["coupled"];
	}

	if (cpuModel.empty() && comModel.empty()) {
		return std::make_unique<CombinedCpuTask>(name, iterations, synchronized, flops, bytes, coupled);
	} else if (cpuModel.empty() && !comModel.empty()) {
		return std::make_unique<CombinedCpuTask>(name, iterations, synchronized, flops, comModel, comPattern,
												 coupled);
	} else if (!cpuModel.empty() && comModel.empty()) {
		return std::make_unique<CombinedCpuTask>(name, iterations, synchronized, cpuModel, cpuPattern, bytes,
												 coupled);
	} else {
		return std::make_unique<CombinedCpuTask>(name, iterations, synchronized, cpuModel, cpuPattern, comModel,
												 comPattern, coupled);
	}

}

std::unique_ptr<Task>
Utility::createSequenceTask(nlohmann::json jsonTask, const std::string& name, const std::string& iterations,
							bool synchronized, const std::map<std::string, std::string>& arguments, int numNodes,
							int numGpusPerNode) {
	std::deque<std::unique_ptr<Task>> tasks;
	for (auto& task: jsonTask["tasks"]) {
		tasks.push_back(readTask(task, arguments, numNodes, numGpusPerNode));
	}
	return std::make_unique<SequenceTask>(name, iterations, synchronized, std::move(tasks));
}

double Utility::evaluateFormula(const std::string& model, int numNodes, int numGpusPerNode) {
	exprtk::parser<double> parser;
	exprtk::expression<double> expression;
	std::string substitutedModel = std::regex_replace(model, std::regex("num_nodes"), std::to_string(numNodes));
	substitutedModel = std::regex_replace(substitutedModel, std::regex("num_gpus_per_node"),
										  std::to_string(numGpusPerNode));
	substitutedModel = std::regex_replace(substitutedModel, std::regex("num_gpus"),
										  std::to_string(numNodes * numGpusPerNode));
	if (parser.compile(substitutedModel, expression)) {
		return expression.value();
	} else {
		xbt_die("Performance model %s not valid", model.c_str());
	}
}

std::vector<double> Utility::createVector(double size, VectorPattern pattern, int numNodes) {
	std::vector<double> sizes(numNodes);
	if (pattern == UNIFORM) {
		std::fill(std::begin(sizes), std::end(sizes), size);
	} else if (pattern == EVEN_RANKS) {
		int participatingNodes = numNodes % 2 == 0 ? numNodes / 2 : numNodes / 2 + 1;
		double sizePerNode = size / participatingNodes;
		for (int i = 0; i < numNodes; i+=2) {
			sizes[i] = sizePerNode;
		}
	} else if (pattern == ODD_RANKS) {
		int participatingNodes = numNodes / 2;
		double sizePerNode = size / participatingNodes;
		for (int i = 1; i < numNodes; i+=2) {
			sizes[i] = sizePerNode;
		}
	} else if (pattern == ROOT_ONLY) {
		sizes[0] = size;
	} else if (pattern == ALL_RANKS) {
		double sizePerNode = size / numNodes;
		std::fill(std::begin(sizes), std::end(sizes), sizePerNode);
	}
	return sizes;
}

std::vector<double>
Utility::createVector(const std::string& model, VectorPattern pattern, int numNodes, int numGpusPerNode) {
	double size = evaluateFormula(model, numNodes, numGpusPerNode);
	return createVector(size, pattern, numNodes);
}

std::vector<double>
Utility::createMatrix(double size, MatrixPattern pattern, int numNodes) {
	if (numNodes == 1) {
		return {0};
	}
	std::vector<double> sizes(numNodes * numNodes);
	double payload = size;
	if (pattern == ALL_TO_ALL) {
		payload = size / (numNodes * numNodes - numNodes);
		for (int i = 0; i < numNodes; ++i) {
			for (int j = 0; j < numNodes; ++j) {
				if (i == j) continue;
				sizes[i * numNodes + j] = payload;
			}
		}
	} else if (pattern == GATHER) {
		payload = size / numNodes;
		for (int i = 1; i < numNodes; ++i) {
			sizes[i * numNodes] = payload;
		}
	} else if (pattern == SCATTER) {
		payload = size / numNodes;
		for (int i = 1; i < numNodes; ++i) {
			sizes[i] = payload;
		}
	} else if (pattern == RING) {
		payload = size / (numNodes * 2);
		for (int i = 0; i < numNodes; ++i) {
			sizes[i * numNodes + EUCLIDIAN_MOD(i - 1, numNodes)] += payload;
			sizes[i * numNodes + EUCLIDIAN_MOD(i + 1, numNodes)] += payload;
		}
	} else if (pattern == RING_CLOCKWISE) {
		payload = size / numNodes;
		for (int i = 0; i < numNodes; ++i) {
			sizes[i * numNodes + EUCLIDIAN_MOD(i + 1, numNodes)] += payload;
		}
	} else if (pattern == RING_COUNTER_CLOCKWISE) {
		payload = size / numNodes;
		for (int i = 0; i < numNodes; ++i) {
			sizes[i * numNodes + EUCLIDIAN_MOD(i - 1, numNodes)] += payload;
		}
	} else if (pattern == MASTER_WORKER) {
		payload = size / ((numNodes - 1) / (double) 2);
		for (int i = 1; i < numNodes; ++i) {
			sizes[0 * numNodes + i] = payload;
			sizes[i * numNodes + 0] = payload;
		}
	} else {
		xbt_die("Unsupported CPU communication pattern %s", asString(pattern).c_str());
	}
	return sizes;
}

std::vector<double>
Utility::createMatrix(const std::string& model, MatrixPattern pattern, int numNodes, int numGpusPerNode) {
	double size = evaluateFormula(model, numNodes, numGpusPerNode);
	return createMatrix(size, pattern, numNodes);
}

std::pair<std::vector<double>, std::vector<double>>
Utility::createMatrices(double size, MatrixPattern pattern, int numNodes, int numGpusPerNode) {

	double intraNodeComSize;
	double interNodeComSize;
	int numGpus = numNodes * numGpusPerNode;
	if (numGpusPerNode == 1) {
		intraNodeComSize = 0;
		interNodeComSize = size;
	} else if (pattern == ALL_TO_ALL) {
		double payloadPerCommunication = size / (numGpus * numGpus - numGpus);
		double gpusToCommunicatePerNode = numGpusPerNode - 1;
		intraNodeComSize = payloadPerCommunication * gpusToCommunicatePerNode * gpusToCommunicatePerNode;
		interNodeComSize = payloadPerCommunication * (numNodes * numGpusPerNode - numGpusPerNode);
	} else if (pattern == RING) {
		double payloadPerCommunication = size / (numGpus * 2);
		if (numNodes == 1) {
			intraNodeComSize = payloadPerCommunication * numGpusPerNode * 2;
		} else {
			intraNodeComSize = payloadPerCommunication * (numGpusPerNode - 1) * 2;
		}
		interNodeComSize = payloadPerCommunication * numNodes * 2;
	} else if (pattern == RING_CLOCKWISE || pattern == RING_COUNTER_CLOCKWISE) {
		double payloadPerCommunication = size / numGpus;
		if (numNodes == 1) {
			intraNodeComSize = payloadPerCommunication * numGpusPerNode;
		} else {
			intraNodeComSize = payloadPerCommunication * (numGpusPerNode - 1);
		}
		interNodeComSize = payloadPerCommunication * numNodes * 2;
	} else {
		xbt_die("Unsupported GPU communication pattern %s", asString(pattern).c_str());
	}

	std::vector<double> intraNodeCommunication = createMatrix(intraNodeComSize, pattern, numGpusPerNode);
	std::vector<double> interNodeCommunication = createMatrix(interNodeComSize, pattern, numNodes);

	return {intraNodeCommunication, interNodeCommunication};
}

std::pair<std::vector<double>, std::vector<double>>
Utility::createMatrices(const std::string& model, MatrixPattern pattern, int numNodes, int numGpusPerNode) {
	double size = evaluateFormula(model, numNodes, numGpusPerNode);
	return createMatrices(size, pattern, numNodes, numGpusPerNode);
}

std::vector<std::unique_ptr<Job>> Utility::readJobs(const std::string& jobsFile) {
	std::ifstream stream(jobsFile);
	nlohmann::json json = nlohmann::json::parse(stream);
	std::vector<std::unique_ptr<Job>> jobs;
	for (auto& job: json["jobs"]) {
		JobType jobType = parseJobType(job["type"]);
		double walltime = 0;
		if (job["walltime"].is_number_unsigned()) {
			walltime = job["walltime"];
		}
		int numGpusPerNode = 0;
		if (job["num_gpus_per_node"].is_number_unsigned()) {
			numGpusPerNode = job["num_gpus_per_node"];
		}
		int numNodesMin = 0;
		if (job["num_nodes_min"].is_number_unsigned()) {
			numNodesMin = job["num_nodes_min"];
		}
		int numNodesMax = 0;
		if (job["num_nodes_max"].is_number_unsigned()) {
			numNodesMax = job["num_nodes_max"];
		}
		int numGpusPerNodeMin = 0;
		if (job["num_gpus_per_node_min"].is_number_unsigned()) {
			numGpusPerNodeMin = job["num_gpus_per_node_min"];
		}
		int numGpusPerNodeMax = 0;
		if (job["num_gpus_per_node_max"].is_number_unsigned()) {
			numGpusPerNodeMax = job["num_gpus_per_node_max"];
		}
		std::map<std::string, std::string> arguments;
		if (!job["arguments"].is_null()) {
			arguments = readStringMap(job["arguments"]);
		}
		std::map<std::string, std::string> attributes;
		if (!job["attributes"].is_null()) {
			attributes = readStringMap(job["attributes"]);
		}
		if (jobType == RIGID) {
			if (job["num_nodes"].is_null()) {
				xbt_die("Requested number of nodes has to be specified for rigid jobs");
			}
			int numNodes = job["num_nodes"];
			if (numNodes < 1) {
				xbt_die("Requested number of nodes can not be less than 1 for rigid jobs");
			}
			jobs.push_back(
					std::make_unique<Job>(walltime, job["num_nodes"], numGpusPerNode,
										  job["submit_time"], arguments, attributes,
										  readWorkload(job["application_model"], arguments, job["num_nodes"],
													   numGpusPerNode)));
		} else {
			jobs.push_back(
					std::make_unique<Job>(walltime, jobType, numNodesMin, numNodesMax,
										  numGpusPerNodeMin, numGpusPerNodeMax, job["submit_time"], arguments,
										  attributes,
										  readWorkload(job["application_model"], arguments)));
		}
	}
	return jobs;
}
