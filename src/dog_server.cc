#include "dog_server.h"
#include "utils.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#ifdef DEBUG
#include <cstdio>
#define LOG(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define LOG(fmt, ...)
#endif

namespace dlstools {

bool DogServer::Load(const std::string &path) {
  // open the input stream
  std::ifstream in(path);
  if (!in) {
    LOG("Error opening file: %s\n", path.c_str());
    return false;
  }

  // read the file line by line
  std::string line;
  while (std::getline(in, line)) {
    // trim whitespace from the line
    line.erase(0, line.find_first_not_of(" \t\n\r"));
    line.erase(line.find_last_not_of(" \t\n\r") + 1);

    // skip empty lines and comments
    if (line.empty() || line[0] == '#') {
      continue;
    }

    // split the line by columns
    size_t pos = line.find_first_of(" \t");
    if (pos == std::string::npos) {
      LOG("Invalid line format: %s\n", line.c_str());
      continue;
    }

    std::string value = line.substr(0, pos);
    std::string key = line.substr(pos + 1);

    // convert the value to uint32_t
    uint32_t val = static_cast<uint32_t>(std::stoul(value, nullptr, 16));

    // store the key-value pair in the map
    mTable[key] = val;
  }

  // print the total number of entries loaded
  LOG("Loaded %zu entries from %s\n", mTable.size(), path.c_str());

  // close the input stream
  in.close();
  return true;
}

uint32_t DogServer::Convert(const char *rq, uint32_t rq_size) {
  return Convert(std::string(rq, rq_size));
}

uint32_t DogServer::Convert(const std::string &rq) {
  uint32_t result = 0xFFFFFFFF;
  std::string hex_rq = utils::hexalify(rq);
  if (mTable.count(hex_rq)) {
    result = mTable[hex_rq];
  }
  return result;
}

} // namespace dlstools
