/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#ifndef ELASTISIM_CONFIGURATION_H
#define ELASTISIM_CONFIGURATION_H


#include <json.hpp>

class Configuration {

private:
	static nlohmann::json configuration;
	static bool initialized;

public:

	static void init(const std::string& configurationFilePath);

	static nlohmann::basic_json<> get(const std::string& key);

	static bool exists(const std::string& key);

	static bool getBoolIfExists(const std::string& key);

};


#endif //ELASTISIM_CONFIGURATION_H
