#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

namespace dlstools {
namespace utils {

std::string hexalify(const uint32_t value);
std::string hexalify(const std::string& str);
std::string toUpper(const std::string& str);
bool startsWith(const std::string& str, const std::string& prefix);
bool getRecursiveDirectoryListing(const std::string& path, std::vector<std::string>& file_list);
std::string getKeysFilePath(void);
std::string getCardFilePath(void);
std::string getScriptsPath(void);
std::string getSettingsPath(void);

} // namespace utils
} // namespace dlstools

#endif // UTILS_H