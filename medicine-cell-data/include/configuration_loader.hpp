#ifndef CONFIGURATION_LOADER_HPP
#define CONFIGURATION_LOADER_HPP

#include "configuration.hpp"

bool loadConfiguration(
    const char* json,
    RuntimeConfiguration& configuration
);

#endif