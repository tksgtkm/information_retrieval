#include <cassert>
#include <cstring>
#include <iostream>
#include "configurator.h"

void test_basic_key_value_param() {
    const char *argv[] = {
        "program",
        "FOO=bar",
        "--BAZ=qux"
    };

    initializeConfiguratorFromCommandLineParameters(3, argv);

    char buffer[128];

    assert(getConfigurationValue("FOO", buffer));
    assert(std::string(buffer) == "bar");

    assert(getConfigurationValue("BAZ", buffer));
    assert(std::string(buffer) == "qux");

    std::cout << "test_basic_key_value_param passed.\n";
}

void test_config_param_takes_precedence() {

    const char *argv[] = {
        "program",
        "CONFIG=/dev/null",
        "X=1"
    };

    initializeConfiguratorFromCommandLineParameters(3, argv);

    char buffer[128];
    assert(getConfigurationValue("X", buffer));
    assert(std::string(buffer) == "1");

    std::cout << "test_config_param_takes_precedence passed.\n";
}

void test_env_variable_used_if_no_config_param() {
    setenv("RETRIEVAL_CONFIG_FILE", "/dev/null", 1);

    const char *argv[] = {
        "program",
        "KEY=value"
    };

    initializeConfiguratorFromCommandLineParameters(2, argv);

    char buffer[128];
    assert(getConfigurationValue("KEY", buffer));
    assert(std::string(buffer) == "value");

    std::cout << "test_env_variable_used_if_no_config_param passed.\n";

    unsetenv("RETRIEVAL_CONFIG_FILE");
}


int main() {
    test_basic_key_value_param();
    test_config_param_takes_precedence();
    test_env_variable_used_if_no_config_param();
    std::cout << "All configurator tests passed.\n";
}