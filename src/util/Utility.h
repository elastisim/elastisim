/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_UTILITY_H
#define ELASTISIM_UTILITY_H

#include <deque>
#include <json.hpp>
#include "Task.h"
#include "CombinedTask.h"
#include "IoTask.h"
#include "Job.h"


class Task;

class Phase;

class Workload;

class Job;

class Utility {

private:
	static std::string toLower(const std::string& string);

	static JobType parseJobType(const std::string& jobType);

	static std::string asString(VectorPattern pattern);

	static VectorPattern asVectorPattern(const std::string& pattern);

	static std::string asString(MatrixPattern pattern);

	static MatrixPattern asMatrixPattern(const std::string& pattern);

	static std::string applyArguments(const std::string& model, const std::map<std::string, std::string>& arguments);

	static std::map<std::string, std::string> readStringMap(nlohmann::json jsonMap);

	static std::unique_ptr<Task>
	readTask(nlohmann::json jsonTask, const std::map<std::string, std::string>& arguments, int numNodes,
			 int numGpusPerNode);

	static std::unique_ptr<Phase>
	readPhase(nlohmann::json jsonPhase, const std::map<std::string, std::string>& arguments, int numNodes,
			  int numGpusPerNode);

	static std::unique_ptr<Phase>
	readOneTimePhase(nlohmann::json jsonPhase, const std::map<std::string, std::string>& arguments,
					 bool mandatoryBarrier, int numNodes, int numGpusPerNode);

	static std::unique_ptr<Workload>
	readWorkload(const std::string& workloadFile, const std::map<std::string, std::string>& arguments,
				 int numNodes = 0, int numGpusPerNode = 0);

	template<typename T>
	static std::unique_ptr<T>
	createDelayTask(nlohmann::json jsonTask, const std::string& name, const std::string& iterations, bool synchronized,
					const std::map<std::string, std::string>& arguments, int numNodes, int numGpusPerNode);

	template<typename T>
	static std::unique_ptr<T>
	createIoTask(nlohmann::json jsonTask, const std::string& name, const std::string& iterations, bool synchronized,
				 const std::map<std::string, std::string>& arguments, int numNodes, int numGpusPerNode);

	static std::unique_ptr<Task>
	createCombinedGpuTask(nlohmann::json jsonTask, const std::string& name, const std::string& iterations,
						  bool synchronized,
						  const std::map<std::string, std::string>& arguments, int numNodes, int numGpusPerNode);

	static std::unique_ptr<Task>
	createCombinedCpuTask(nlohmann::json jsonTask, const std::string& name, const std::string& iterations,
						  bool synchronized,
						  const std::map<std::string, std::string>& arguments, int numNodes, int numGpusPerNode);

	static std::unique_ptr<Task>
	createSequenceTask(nlohmann::json jsonTask, const std::string& name, const std::string& iterations,
					   bool synchronized,
					   const std::map<std::string, std::string>& arguments, int numNodes, int numGpusPerNode);

public:
	static double evaluateFormula(const std::string& model, int numNodes, int numGpusPerNode);

	static std::vector<double> createVector(double size, VectorPattern pattern, int numNodes);

	static std::vector<double>
	createVector(const std::string& model, VectorPattern pattern, int numNodes, int numGpusPerNode);

	static std::vector<double> createMatrix(double size, MatrixPattern pattern, int numNodes);

	static std::vector<double>
	createMatrix(const std::string& model, MatrixPattern pattern, int numNodes, int numGpusPerNode);

	static std::pair<std::vector<double>, std::vector<double>>
	createMatrices(double size, MatrixPattern pattern, int numNodes, int numGpusPerNode);

	static std::pair<std::vector<double>, std::vector<double>>
	createMatrices(const std::string& model, MatrixPattern pattern, int numNodes, int numGpusPerNode);

	static std::vector<std::unique_ptr<Job>> readJobs(const std::string& jobsFile);

};

#endif //ELASTISIM_UTILITY_H
