#ifndef DOG_SERVER_H
#define DOG_SERVER_H

#include <string>
#include <unordered_map>
#include <cstdint>

namespace dlstools {
class DogServer {
public:
    DogServer() = default;
    ~DogServer() = default;
    DogServer(const DogServer&) = delete;
    DogServer& operator=(const DogServer&) = delete;
    DogServer(DogServer&&) = delete;
    DogServer& operator=(DogServer&&) = delete;


    bool Load(const std::string& path);
    uint32_t Convert(const char *rq, uint32_t rq_size);
    uint32_t Convert(const std::string& rq);

private:
    std::unordered_map<std::string,uint32_t> mTable;
};
} // namespace dlstools
#endif // DOG_SERVER_H