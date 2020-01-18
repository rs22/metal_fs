#pragma once

#include <stdint.h>

#include <cxxopts.hpp>

namespace metal {

class RegisteredAgent;

class DatagenOperator {
 public:
  static std::string id() { return "datagen"; }
  static bool isDatagenAgent(const RegisteredAgent &agent);
  static void validate(RegisteredAgent &agent);
  static bool isProfilingEnabled(RegisteredAgent &agent);
  static uint64_t datagenLength(RegisteredAgent &agent);

 protected:
  static cxxopts::ParseResult parseOptions(RegisteredAgent &agent);
};

class DevNullFile {
 public:
  static bool isNullOutput(const RegisteredAgent &agent);
};

}  // namespace metal
