/*
 * This file is part of the ElastiSim software.
 *
 * Copyright (c) 2022, Technical University of Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of the 3-Clause
 * BSD License. See the LICENSE file in the base directory for details.
 *
 */

#include <xbt/asserts.h>
#include <fstream>
#include "Configuration.h"

nlohmann::json Configuration::configuration;
bool Configuration::initialized = false;

void Configuration::init(const std::string& configurationFilePath) {
	if (!initialized) {
		std::ifstream stream(configurationFilePath);
		configuration = nlohmann::json::parse(stream);
		initialized = true;
	} else {
		xbt_die("Configuration already initialized");
	}
}

nlohmann::basic_json<> Configuration::get(const std::string& key) {
	return configuration[key];
}

bool Configuration::exists(const std::string& key) {
	return !configuration[key].is_null();
}

bool Configuration::getBoolIfExists(const std::string& key) {
	return configuration[key].is_boolean() && configuration[key];
}
