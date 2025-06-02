#include "card_emulator.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <queue>
#include <vector>

#ifdef DEBUG
#include <cstdio>
#define LOG(fmt, ...) printf(fmt __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG(fmt, ...)
#endif

namespace dlstools {

enum class card_emulator_cmd {
  CMD_READ_CARD_METADATA = 0x53,
  CMD_READ_SECTOR = 0x52,
  CMD_WRITE_SECTOR = 0x57,
};

CardEmulator::CardEmulator()
    : mCardFilePath(""), mId(0x01), mIsInitialized(false) {
  // Setup the sectors
  std::memset(mSectors, 0x00, sizeof(mSectors));
  // setup special sectors
  mSectors[1][4] =
      0x07; // sector 1, offset 5 : this is checked by the game to be 0x07
  mSectors[1][3] =
      0xFF; // sector 1, offset 6 : this is checked by the game to be non 0
  // Initialize the parser state
  mParserState = parser_state::WAITING_FOR_START;
}

CardEmulator::~CardEmulator() {
  if (mIsInitialized) {
    save(mCardFilePath);
  }
}

bool CardEmulator::init(const std::string &path) {

  if (!load(path)) {
    LOG("Failed to open card file, creating a new one\n");

    std::memset(mSectors, 0x00, sizeof(mSectors));
    mSectors[1][4] =
        0x07; // sector 1, offset 5 : this is checked by the game to be 0x07
    mSectors[1][3] =
        0xFF; // sector 1, offset 6 : this is checked by the game to be non 0

    // then write the card to the file
    if (save(path) != 0) {
      LOG("Failed to write card file\n");
      return false;
    }
  }
  mCardFilePath = path;
  mIsInitialized = true;
  return true;
}

int CardEmulator::tx(void *buf, size_t count) {
  uint8_t *data = (uint8_t *)buf;

  for (size_t i = 0; i < count; i++) {
    mTxQueue.push(data[i]);
  }

  // hacky, run the state machine here so we don't care for working with threads
  // this is not the best way to do it, but it works for now
  // additionlly, in a future this should help to decouple the reader and a
  // server to enable customization of the card, emulating the card ejection and
  // stuff like that
  // TODO(ROAD24): run this in a separate thread as a server
  stateMachine();
  return count;
}

int CardEmulator::rx(void *buf, size_t count) {
  int bytesToRead = std::min(count, mRxQueue.size());

  uint8_t *data = (uint8_t *)buf;
  for (int i = 0; i < bytesToRead; i++) {
    data[i] = mRxQueue.front();
    mRxQueue.pop();
  }
  return bytesToRead;
}

bool CardEmulator::save(const std::string &path) {
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  if (!file) {
    LOG("Failed to open card file for writing\n");
    return false;
  }

  // Write mId safely as uint32_t
  file.write(reinterpret_cast<const char *>(&mId), sizeof(uint32_t));
  if (!file) {
    LOG("Error writing card ID\n");
    return false;
  }

  // Write sectors safely
  for (const auto &sector : mSectors) {
    file.write(reinterpret_cast<const char *>(sector), CARD_SECTOR_SIZE);
    if (!file) {
      LOG("Error writing sector data\n");
      return false;
    }
  }

  return true;
}

bool CardEmulator::load(const std::string &path) {
  LOG("CardEmulator::load(%s)\n", path.c_str());

  std::ifstream file(path, std::ios::binary);
  if (!file) {
    LOG("Failed to open card file for reading\n");
    return false;
  }

  // Read mId safely as a uint32_t
  uint32_t id = 0;
  file.read(reinterpret_cast<char *>(&id), sizeof(uint32_t));

  // Ensure sectors array size is correct before reading
  if (!file) {
    LOG("Failed to read card ID\n");
    return false;
  }

  mId = id;
  // Read the sectors safely
  for (auto &sector : mSectors) {
    file.read(reinterpret_cast<char *>(sector), CARD_SECTOR_SIZE);
    if (!file) {
      LOG("Error reading sector data\n");
      return false;
    }
  }

  return true;
}

bool CardEmulator::readSector(std::vector<uint8_t> &data, uint8_t sector) {
  if (sector >= CARD_SECTORS) {
    return false; // Invalid sector or count
  }

  data.resize(CARD_SECTOR_SIZE);
  data.assign(mSectors[sector], mSectors[sector] + CARD_SECTOR_SIZE);

  return true;
}

bool CardEmulator::writeSector(const std::vector<uint8_t> &data,
                               uint8_t sector) {

  if (sector >= CARD_SECTORS) {
    return false; // Invalid sector or count
  }

  if (data.size() != CARD_SECTOR_SIZE) {
    LOG("Invalid data size for sector %02x: expected %d, got %zu\n", sector,
        CARD_SECTOR_SIZE, data.size());
    return false; // Invalid data size
  }

  if (std::memcmp(mSectors[sector], data.data(), data.size()) == 0) {
    LOG("%s() No changes to write in sector %02x\n", __func__, sector);
    return true; // No changes
  }

  std::memcpy(mSectors[sector], data.data(), data.size());

  if (!save(mCardFilePath)) {
    LOG("Failed to write card file\n");
    return false;
  }
  return true;
}

uint32_t CardEmulator::getId() const { return mId; }

void CardEmulator::setId(uint32_t id) { mId = id; }

void CardEmulator::stateMachineReset() {
  mParserState = parser_state::WAITING_FOR_START;
}

void CardEmulator::stateMachine() {
  static constexpr uint8_t START_BYTE = 0x02;
  static constexpr uint8_t END_BYTE = 0x03;
  static std::vector<uint8_t> dataBuffer;
  static uint8_t receivedBytes = 0;
  static uint8_t packetLength = 0;
  static uint8_t command = 0;
  static uint8_t arg = 0;

  while (!mTxQueue.empty()) {
    uint8_t byte = mTxQueue.front();
    mTxQueue.pop();

    switch (mParserState) {
    case parser_state::WAITING_FOR_START:

      if (byte == START_BYTE) {
        receivedBytes = 1;
        packetLength = 0;
        mParserState = parser_state::WAITING_FOR_LENGTH;
      }
      break;

    case parser_state::WAITING_FOR_LENGTH:
      // We should have at least 6 bytes: start byte, length byte, command byte,
      // subcommand byte, data bytes, checksum byte, end byte
      if (byte < 4) {
        LOG("Invalid length byte: %02x\n", byte);
        stateMachineReset();
        break;
      }
      packetLength =
          byte + 2; // +2 because we included the start an length byte itself
      receivedBytes++;
      dataBuffer.clear();
      mParserState = parser_state::WAITING_FOR_CMD;
      break;

    case parser_state::WAITING_FOR_CMD:
      command = byte;
      receivedBytes++;
      mParserState = parser_state::WAITING_FOR_SUBCMD;
      break;

    case parser_state::WAITING_FOR_SUBCMD:
      arg = byte;
      receivedBytes++;

      if ((receivedBytes + 2) < packetLength) {
        // We expect more data, so we move to data state
        mParserState = parser_state::WAITING_FOR_DATA;
      } else {
        // No data expected, move to checksum state
        mParserState = parser_state::WAITING_FOR_CHECKSUM;
      }
      break;

    case parser_state::WAITING_FOR_DATA:
      receivedBytes++;
      dataBuffer.push_back(byte);
      if ((receivedBytes + 2) >= packetLength) {
        // No more data expected, move to checksum state
        mParserState = parser_state::WAITING_FOR_CHECKSUM;
      }
      break;

    case parser_state::WAITING_FOR_CHECKSUM:
      // game sends a 0xFF byte, but It doesn't care about it
      // and I'm assuming it is the checksum byte since that's the standard for
      // many serial protocols so we just ignore it
      mParserState = parser_state::WAITING_FOR_END;
      break;

    case parser_state::WAITING_FOR_END:
      if (byte == END_BYTE) {
        // Command complete, process it
        processCommand(command, arg, dataBuffer);
      }

      stateMachineReset();
      break;
    default:
      LOG("Unknown parser state: %d\n", static_cast<int>(mParserState));
      stateMachineReset();
      break;
    }
  }
}

void CardEmulator::processCommand(uint8_t command, uint8_t arg,
                                  const std::vector<uint8_t> &data) {
  switch (static_cast<card_emulator_cmd>(command)) {
  case card_emulator_cmd::CMD_READ_CARD_METADATA:
    onReadCardMetadata();
    break;
  case card_emulator_cmd::CMD_READ_SECTOR:
    onReadSector(arg);
    break;
  case card_emulator_cmd::CMD_WRITE_SECTOR:
    onWriteSector(data, arg);
    break;
  default:
    LOG("Unknown command: %02x %02x\n", command, arg);
    break;
  }
}

bool CardEmulator::onReadCardMetadata() {
  LOG("Reading card metadata\n");
  mRxQueue.push(0x02); // Start byte
  mRxQueue.push(0x0A); // Length byte (12 bytes for header + 1 byte for command
                       // + 1 byte for checksum)
  mRxQueue.push(static_cast<uint8_t>(
      card_emulator_cmd::CMD_READ_CARD_METADATA)); // Command byte
  // I dont know what this is for, not sure if the game cares about it
  mRxQueue.push(0xFF);
  mRxQueue.push(0x01);
  // Push the card ID
  mRxQueue.push(static_cast<uint8_t>(mId & 0xFF));
  mRxQueue.push(static_cast<uint8_t>((mId >> 8) & 0xFF));
  mRxQueue.push(static_cast<uint8_t>((mId >> 16) & 0xFF));
  mRxQueue.push(static_cast<uint8_t>((mId >> 24) & 0xFF));
  // I dont know what this is for, not sure if the game cares about it
  mRxQueue.push(0xFF);
  // Push the "checksum" byte
  mRxQueue.push(0xFF); // Placeholder for checksum, game doesn't care about it
  // Add end byte
  mRxQueue.push(0x03); // End of command
  return true;
}

bool CardEmulator::onReadSector(uint8_t sector) {
  if (sector >= CARD_SECTORS) {
    LOG("Invalid sector number: %02x\n", sector);
    return false; // Invalid sector
  }

  std::vector<uint8_t> sectorData(CARD_SECTOR_SIZE);
  if (!readSector(sectorData, sector)) {
    LOG("Failed to read sector %02x\n", sector);
    return false;
  }

  mRxQueue.push(0x02); // Start byte
  mRxQueue.push(0x34); // Length byte (2 bytes for header + 1 byte for command +
                       // 1 byte for checksum)
  mRxQueue.push(
      static_cast<uint8_t>(card_emulator_cmd::CMD_READ_SECTOR)); // Command byte
  mRxQueue.push(sector); // Sector number

  // Push the data to the RX queue
  for (const auto &byte : sectorData) {
    mRxQueue.push(byte);
  }

  // push the "checksum" byte
  mRxQueue.push(0xFF); // Placeholder for checksum, game doesn't care about it
  // Add end byte
  mRxQueue.push(0x03); // End of command
  return true;
}

bool CardEmulator::onWriteSector(const std::vector<uint8_t> &data,
                                 uint8_t sector) {

  if (sector >= CARD_SECTORS) {
    LOG("Invalid sector number: %02x\n", sector);
    return false; // Invalid sector
  }

  if (data.size() != CARD_SECTOR_SIZE) {
    LOG("Invalid data size for sector %02x: expected %d, got %zu\n", sector,
        CARD_SECTOR_SIZE, data.size());
    return false; // Invalid data size
  }

  // NOTE(ROAD24):
  // The game doesn't handle properly the reader protocol
  // so I was not able to implement properly the command handling
  // most of this are assumptions from the game behavior
  // and years of experience with serial protocols for embedded systems
  // so don't expect this to be 100% accurate
  // for example, I still don't know wheter the reader expects a checksum or not
  // or the 0xFF byte is just to detect the end of the command

  mRxQueue.push(0x02); // Start byte
  mRxQueue.push(0x05); // Length byte (5 bytes for header + 1 byte for command +
                       // 1 byte for checksum)
  mRxQueue.push(static_cast<uint8_t>(
      card_emulator_cmd::CMD_WRITE_SECTOR)); // Command byte
  mRxQueue.push(sector);                     // Sector number
  // Push the "ack" byte
  mRxQueue.push(0x01); // Acknowledgment or total sectors written

  // push the "checksum" byte
  mRxQueue.push(0xFF); // Placeholder for checksum, game doesn't care about it
  // Add end byte
  mRxQueue.push(0x03); // End of command

  return true;
}

} // namespace dlstools
