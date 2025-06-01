#ifndef CARD_EMULATOR_H
#define CARD_EMULATOR_H

#include <cstdint>
#include <string>
#include <queue>
#include <vector>

static constexpr int CARD_SECTORS = 0x30;
static constexpr int CARD_SECTOR_SIZE = 48;
static constexpr int MAX_CARD_PATH = 256;

namespace dlshook {

class CardEmulator {
public:
    CardEmulator();
    ~CardEmulator();

    bool init();
    int tx(void *buf, size_t count);
    int rx(void *buf, size_t count);
    uint32_t getId() const;
    void setId(uint32_t id);

private:
    bool readSector(std::vector<uint8_t>& data, uint8_t sector);
    bool writeSector(const std::vector<uint8_t>& data, uint8_t sector);
    int close();
    bool load(const std::string& path);
    bool save(const std::string& path);

    // Command processing methods
    void stateMachine();
    void stateMachineReset();
    void processCommand(uint8_t command, uint8_t subcommand, const std::vector<uint8_t>& data);
    bool onReadCardMetadata();
    bool onReadSector(uint8_t sector);
    bool onWriteSector(const std::vector<uint8_t>& data, uint8_t sector);

    enum class parser_state {
        WAITING_FOR_START,
        WAITING_FOR_LENGTH,
        WAITING_FOR_CMD,
        WAITING_FOR_SUBCMD,
        WAITING_FOR_DATA,
        WAITING_FOR_CHECKSUM,
        WAITING_FOR_END
    };

    std::string mCardFilePath;
    uint8_t mSectors[CARD_SECTORS][CARD_SECTOR_SIZE];
    uint32_t mId;
    std::queue<uint8_t> mRxQueue;
    std::queue<uint8_t> mTxQueue;
    bool mIsInitialized;
    parser_state mParserState;
};
} // namespace dlshook
#endif // CARD_EMULATOR_H