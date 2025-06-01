#include "utils.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

static std::string_view KEYS_PATH = "dls.keys";
static std::string_view KEYS_PATH_ENV_VAR = "DLS_KEYS_PATH";
static std::string_view CARD_PATH_ENV_VAR = "CARD_PATH";
static std::string_view CARD_PATH = "/SAVE/DLS/card.bin";

namespace dlstools {
namespace utils {

std::string hexalify(const uint32_t value) {
    std::stringstream oss;
    oss << std::setw(8) << std::hex << value;
    return oss.str();
}

std::string hexalify(const std::string& str) {
    std::ostringstream oss;
    for (unsigned char c : str) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    return oss.str();
}

std::string toUpper(const std::string& str) {
    std::string upper_str = str;
    std::transform(upper_str.begin(), upper_str.end(), upper_str.begin(),::toupper);
    return upper_str;
}

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.compare(0, prefix.size(), prefix) == 0;
}

bool getRecursiveDirectoryListing(const std::string& path, std::vector<std::string>& file_list) {
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            // Check if the entry is a regular file
            if (entry.is_regular_file()) {
                std::string file_path = entry.path().string();
                file_list.push_back(file_path);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        printf("Error accessing directory: %s\n", e.what());
        return false;
    }

    return true;
}

std::string getKeysFilePath(void) {
    // check if the environment variable is set
    const char* envPath = std::getenv(KEYS_PATH_ENV_VAR.data());
    if (envPath) {
        return std::string(envPath);
    }

    // if not, use the default path
    return std::string(KEYS_PATH);
}

std::string getCardFilePath(void) {
    // check if the environment variable is set
    const char* envPath = std::getenv(CARD_PATH_ENV_VAR.data());
    if (envPath) {
        return std::string(envPath);
    }

    // if not, use the default path
    return std::string(CARD_PATH);
}

} // namespace utils
} // namespace dlstools
