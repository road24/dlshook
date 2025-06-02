// g++ -m32 -fPIC -shared dlshook.cc utils.cc dog_server.cc card_emulator.c
// -std=c++17 -o dlshook.so -ldl -lstdc++fs for building with docker: docker
// pull ckdur/manylinux-glibc212-32bit docker run --rm -it -v $(pwd):/app
// ckdur/manylinux-glibc212-32bit /bin/bash
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>

#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>

#include "card_emulator.h"
#include "dog_server.h"
#include "utils.h"

static void *DogConvertAddress = (void *)0x080BF1D9;
static uint8_t **DogData = (uint8_t **)0xA79B2F4;
static uint8_t *DogDataSize = (uint8_t *)0x0A79B2E0;
static uint32_t *DogResult = (uint32_t *)0xA79B2EC;
static const int CARD_EMULATOR_FD = 0xFFFFFFE;

static dlstools::CardEmulator *gCardEmulator = nullptr;
static dlstools::DogServer *gDogServer = nullptr;

std::unordered_map<std::string, std::string> g_CaseInsensitiveFSMap;
std::unordered_map<std::string, std::string> *g_FolderRedirects = nullptr;

// function pointers for the real functions
static FILE *(*real_fopen)(const char *, const char *) = nullptr;
static int (*real_write)(int, const void *, size_t) = nullptr;
static int (*real_read)(int, void *, size_t) = nullptr;
static int (*real_chdir)(const char *) = nullptr;
static int (*real_open)(const char *path, int flags) = nullptr;
static int (*real_close)(int) = nullptr;
static int (*real_tcgetattr)(int, struct termios *) =
    (int (*)(int, struct termios *))dlsym(RTLD_NEXT, "tcgetattr");
static int (*real_tcsetattr)(int, int, const struct termios *) =
    (int (*)(int, int, const struct termios *))dlsym(RTLD_NEXT, "tcsetattr");
static int (*real_tcflush)(int, int) = (int (*)(int, int))dlsym(RTLD_NEXT,
                                                                "tcflush");
static int (*real_tcdrain)(int) = (int (*)(int))dlsym(RTLD_NEXT, "tcdrain");

extern "C" int chdir(const char *path) {
  if (!real_chdir) {
    real_chdir = (int (*)(const char *))dlsym(RTLD_NEXT, "chdir");
  }
  int result = real_chdir(path);
  if (result == 0) {
    std::vector<std::string> file_list;
    if (dlstools::utils::getRecursiveDirectoryListing(".", file_list)) {
      // push the files into the global filesystem set
      for (auto &file : file_list) {
        // remove the leading "./" if it exists
        if (file.size() > 2 && file[0] == '.' && file[1] == '/') {
          file = file.substr(2);
        }

        // convert the file to upper case and add it to the set
        std::string upper_file = dlstools::utils::toUpper(file);
        g_CaseInsensitiveFSMap[upper_file] = file;
      }
    }
  }
  return result;
}

extern "C" int tcgetattr(int fd, struct termios *termios_p) {
  int result = -1;

  if (real_tcgetattr == nullptr) {
    real_tcgetattr =
        (int (*)(int, struct termios *))dlsym(RTLD_NEXT, "tcgetattr");
  }

  if (fd == CARD_EMULATOR_FD) {
    printf("tcgetattr(%d)\n", fd);
    result = 0;
  } else {
    result = real_tcgetattr(fd, termios_p);
  }
  return result;
}

extern "C" int tcsetattr(int fd, int optional_actions,
                         const struct termios *termios_p) {
  int result = -1;

  if (real_tcsetattr == nullptr) {
    real_tcsetattr = (int (*)(int, int, const struct termios *))dlsym(
        RTLD_NEXT, "tcsetattr");
  }

  if (fd == CARD_EMULATOR_FD) {
    printf("tcsetattr(%d)\n", fd);
    result = 0;
  } else {
    result = real_tcsetattr(fd, optional_actions, termios_p);
  }
  return result;
}

extern "C" int tcflush(int fd, int queue_selector) {
  int result = -1;

  if (real_tcflush == nullptr) {
    real_tcflush = (int (*)(int, int))dlsym(RTLD_NEXT, "tcflush");
  }

  if (fd == CARD_EMULATOR_FD) {
    printf("tcflush(%d)\n", fd);
    result = 0;
  } else {
    result = real_tcflush(fd, queue_selector);
  }
  return result;
}

extern "C" int tcdrain(int fd) {
  int result = -1;

  if (real_tcdrain == nullptr) {
    real_tcdrain = (int (*)(int))dlsym(RTLD_NEXT, "tcdrain");
  }

  if (fd == CARD_EMULATOR_FD) {
    printf("tcdrain(%d)\n", fd);
    result = 0;
  } else {
    result = real_tcdrain(fd);
  }
  return result;
}

extern "C" int open(const char *path, int flags) {
  int fd = -1;

  if (!real_open) {
    real_open = (int (*)(const char *, int))dlsym(RTLD_NEXT, "open");
  }
  std::string path_str(path);

  if (path_str == "/dev/ttyS0") {
    printf("detouring open(%s, %d)\n", path, flags);
    gCardEmulator = new dlstools::CardEmulator();
    if (gCardEmulator->init(dlstools::utils::getCardFilePath())) {
      fd = CARD_EMULATOR_FD;
    }
  } else {
    fd = real_open(path, flags);
  }
  return fd;
}

extern "C" int close(int fd) {
  int result = -1;
  if (real_close == nullptr) {
    real_close = (int (*)(int))dlsym(RTLD_NEXT, "close");
  }

  if (fd == CARD_EMULATOR_FD) {
    printf("closed com fd(%d)\n", fd);
    result = 0;
  } else {
    result = real_close(fd);
  }

  return result;
}

extern "C" int write(int fd, const void *buf, size_t count) {
  int result = count;
  if (real_write == nullptr) {
    real_write = (int (*)(int, const void *, size_t))dlsym(RTLD_NEXT, "write");
  }

  if (fd == CARD_EMULATOR_FD) {
    if (gCardEmulator) {
      result = gCardEmulator->tx((void *)buf, count);
      printf("write(%d, %p, %zu) -> %d : ", fd, buf, count, result);
      for (int i = 0; i < result; i++) {
        printf("%02x ", ((uint8_t *)buf)[i]);
      }
      printf("\n");
    }
  } else {
    result = real_write(fd, buf, count);
  }
  return result;
}

extern "C" int read(int fd, void *buf, size_t count) {
  int result = count;

  if (real_read == nullptr) {
    real_read = (int (*)(int, void *, size_t))dlsym(RTLD_NEXT, "read");
  }

  if (fd == CARD_EMULATOR_FD) {
    if (gCardEmulator) {
      result = gCardEmulator->rx(buf, count);
      printf("read(%d, %p, %zu) -> %d : ", fd, buf, count, result);
      for (int i = 0; i < result; i++) {
        printf("%02x ", ((uint8_t *)buf)[i]);
      }
      printf("\n");
    }
  } else {
    result = real_read(fd, buf, count);
  }
  return result;
}

extern "C" FILE *fopen(const char *c_path, const char *mode) {
  FILE *result = NULL;
  std::string path(c_path);

  if (real_fopen == nullptr) {
    real_fopen =
        (FILE * (*)(const char *, const char *)) dlsym(RTLD_NEXT, "fopen");
  }

  // check if we have a redirect for this path
  auto it =
      std::find_if(g_FolderRedirects->begin(), g_FolderRedirects->end(),
                   [&path](const std::pair<std::string, std::string> &pair) {
                     return dlstools::utils::startsWith(path, pair.first);
                   });

  if (it != g_FolderRedirects->end()) {
    // printf("redirecting %s to %s\n", path.c_str(), it->second.c_str());
    // Redirect the path to the new folder
    path = it->second + path.substr(it->first.size());
  }

  // if open failed, try finding the file in the filesystem map
  if ((result = real_fopen(path.c_str(), mode)) == NULL) {
    std::string upperCasePath = dlstools::utils::toUpper(path);
    if (g_CaseInsensitiveFSMap.count(upperCasePath)) {
      result = real_fopen(g_CaseInsensitiveFSMap[upperCasePath].c_str(), mode);
    }
  }

  if (result == NULL) {
    printf("failed to open %s\n", path.c_str());
  }

  return result;
}

extern "C" uint32_t DogConvert(void) {
  uint32_t result = 0;
  uint32_t rq_size = *DogDataSize;
  const char *rq = (const char *)*DogData;

  if (gDogServer) {
    // convert the data
    *DogResult = gDogServer->Convert(rq, rq_size);
    if (*DogResult == 0xFFFFFFFF) {
      printf("DogConvert () ");
      for (size_t i = 0; i < rq_size; i++) {
        printf("%02x", (unsigned char)rq[i]);
      }
      printf(" -> %08x\n", *DogResult);
      result = 0xFFFFFFFF;
    }
  }
  return result;
}

static void unprotect(void *addr, size_t size) {
  // unprotect the memory region
  mprotect((void *)((int)addr - ((int)addr % size)), size,
           PROT_READ | PROT_WRITE | PROT_EXEC);
}

static void detour_function(void *new_adr, void *addr) {
  int call = (int)((int)new_adr - (int)addr - 5);
  unprotect(addr, 4096);
  *((unsigned char *)(addr)) = 0xE9;
  *((int *)((int)addr + 1)) = call;
}

static void __attribute__((destructor)) cleanup(void) {
  printf("cleaning up stuff\n");
  if (gDogServer) {
    delete gDogServer;
    gDogServer = nullptr;
  }

  if (gCardEmulator) {
    delete gCardEmulator;
    gCardEmulator = nullptr;
  }
  printf("cleanup done\n");
}

void __attribute__((constructor)) initialize(void) {
  printf("initializing stuff\n");
  gDogServer = new dlstools::DogServer();
  gDogServer->Load(dlstools::utils::getKeysFilePath());
  printf("detouring function\n");
  detour_function((void *)&DogConvert, DogConvertAddress);

  // Setup the directory redirects
  // ugly hack, static objects might be initialized after this constructor
  // so we force the initialization of the map here
  g_FolderRedirects = new std::unordered_map<std::string, std::string>();
  g_FolderRedirects->insert(
      std::pair("/SETTINGS/", dlstools::utils::getSettingsPath()));
  g_FolderRedirects->insert(
      std::pair("/SCRIPT/", dlstools::utils::getScriptsPath()));
}
