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
static std::string_view SCRIPTS_PATH_ENV_VAR = "SCRIPTS_PATH";
static std::string_view SCRIPTS_PATH = "SCRIPT/";
static std::string_view SETTINGS_PATH_ENV_VAR = "SETTINGS_PATH";
static std::string_view SETTINGS_PATH = "/SAVE/DLS/";

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
        // printf("Scanning directory: %s\n", path.c_str());
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
    // printf("Found %zu files in directory: %s\n", file_list.size(), path.c_str());
    return true;
}

std::string getKeysFilePath(void) {
    return getEnvOrDefault(KEYS_PATH_ENV_VAR.data(), KEYS_PATH.data()  );
}

std::string getCardFilePath(void) {
    return getEnvOrDefault(CARD_PATH_ENV_VAR.data(), CARD_PATH.data());
}

std::string getScriptsPath(void) {
    return getEnvOrDefault(SCRIPTS_PATH_ENV_VAR.data(), SCRIPTS_PATH.data());
}

std::string getSettingsPath(void) {
    return getEnvOrDefault(SETTINGS_PATH_ENV_VAR.data(), SETTINGS_PATH.data());
}

std::string getEnvOrDefault(const std::string& envVar, const std::string& defaultValue) {
    // printf("%s(%s, %s)\n", __func__, envVar.c_str(), defaultValue.c_str());
    std::string value(defaultValue);
    const char* envPath = std::getenv(envVar.c_str());
    if (envPath) {
        value = std::string(envPath);
    }
    // printf("Returning value: %s\n", value.c_str());
    return value;
}

} // namespace utils
} // namespace dlstools
