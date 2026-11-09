#include "zucchini_standalone.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

namespace {

void Usage(const char* program) {
  std::cerr
      << "Usage:\n"
      << "  " << program
      << " gen [--equivalence-limit=N --fragmentation-limit=N] "
         "[--equivalence-iterations=N] [--adaptive-equivalence] "
         "OLD NEW PATCH\n"
      << "  " << program << " raw    OLD NEW PATCH\n"
      << "  " << program << " apply  OLD PATCH NEW\n"
      << "  " << program << " verify OLD PATCH EXPECTED_NEW\n\n"
      << "The command names may also be written as -gen, -raw, -apply, "
         "and -verify.\n"
      << "--equivalence-iterations=0 preserves the format default; a positive "
         "value caps the normal number of refinement passes.\n"
      << "--adaptive-equivalence applies the 40000/15000 count/fragmentation "
         "gate after pass 2 on the normal four-pass path.\n";
}

std::string_view Normalize(std::string_view command) {
  if (!command.empty() && command.front() == '-')
    command.remove_prefix(1);
  return command;
}

bool ParseLimit(std::string_view option,
                std::string_view prefix,
                uint32_t* limit) {
  if (option.size() < prefix.size() || option.substr(0, prefix.size()) != prefix)
    return false;

  option.remove_prefix(prefix.size());
  if (option.empty())
    return false;

  std::string value(option);
  char* end = nullptr;
  errno = 0;
  const unsigned long parsed = std::strtoul(value.c_str(), &end, 10);
  if (errno != 0 || end == value.c_str() || *end != '\0' ||
      parsed > std::numeric_limits<uint32_t>::max()) {
    return false;
  }

  *limit = static_cast<uint32_t>(parsed);
  return true;
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc < 5 || argc > 9) {
    Usage(argv[0]);
    return ZUCCHINI_STATUS_INVALID_PARAM;
  }

  const std::string_view command = Normalize(argv[1]);
  int status = ZUCCHINI_STATUS_INVALID_PARAM;

  if (command == "gen") {
    uint32_t equivalence_limit = 0;
    uint32_t fragmentation_limit = 0;
    uint32_t equivalence_iterations = 0;
    bool have_equivalence_limit = false;
    bool have_fragmentation_limit = false;
    bool have_equivalence_iterations = false;
    bool adaptive_equivalence = false;

    const int path_index = argc - 3;
    for (int i = 2; i < path_index; ++i) {
      if (!have_equivalence_limit &&
          ParseLimit(argv[i], "--equivalence-limit=", &equivalence_limit)) {
        have_equivalence_limit = true;
      } else if (!have_fragmentation_limit &&
                 ParseLimit(argv[i], "--fragmentation-limit=",
                            &fragmentation_limit)) {
        have_fragmentation_limit = true;
      } else if (!have_equivalence_iterations &&
                 ParseLimit(argv[i], "--equivalence-iterations=",
                            &equivalence_iterations)) {
        have_equivalence_iterations = true;
      } else if (!adaptive_equivalence &&
                 std::string_view(argv[i]) == "--adaptive-equivalence") {
        adaptive_equivalence = true;
      } else {
        Usage(argv[0]);
        return ZUCCHINI_STATUS_INVALID_PARAM;
      }
    }

    if (have_equivalence_limit != have_fragmentation_limit) {
      Usage(argv[0]);
      return ZUCCHINI_STATUS_INVALID_PARAM;
    }

    if (adaptive_equivalence && equivalence_iterations != 0) {
      std::cerr << "--adaptive-equivalence cannot be combined with a nonzero "
                   "--equivalence-iterations cap.\n";
      return ZUCCHINI_STATUS_INVALID_PARAM;
    }

    status = zucchini_generate_file_with_adaptive_options(
        argv[path_index], argv[path_index + 1], argv[path_index + 2],
        equivalence_limit, fragmentation_limit, equivalence_iterations,
        adaptive_equivalence ? 1 : 0);
  } else if (command == "raw") {
    if (argc != 5) {
      Usage(argv[0]);
      return ZUCCHINI_STATUS_INVALID_PARAM;
    }
    status = zucchini_generate_raw_file(argv[2], argv[3], argv[4]);
  } else if (command == "apply") {
    if (argc != 5) {
      Usage(argv[0]);
      return ZUCCHINI_STATUS_INVALID_PARAM;
    }
    status = zucchini_apply_file(argv[2], argv[3], argv[4]);
  } else if (command == "verify") {
    if (argc != 5) {
      Usage(argv[0]);
      return ZUCCHINI_STATUS_INVALID_PARAM;
    }
    status = zucchini_verify_file(argv[2], argv[3], argv[4]);
  } else {
    Usage(argv[0]);
    return ZUCCHINI_STATUS_INVALID_PARAM;
  }

  if (status != ZUCCHINI_STATUS_SUCCESS) {
    std::cerr << "zucchini: " << zucchini_status_string(status);
    const char* detail = zucchini_last_error();
    if (detail != nullptr && detail[0] != '\0')
      std::cerr << ": " << detail;
    std::cerr << '\n';
  }
  return status;
}
