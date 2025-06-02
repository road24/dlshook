// g++ -m32 -fPIC -shared dlshook.cc utils.cc dog_server.cc card_emulator.c
// -std=c++17 -o dlshook.so -ldl -lstdc++fs for building with docker: docker
// pull ckdur/manylinux-glibc212-32bit docker run --rm -it -v $(pwd):/app
// ckdur/manylinux-glibc212-32bit /bin/bash
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "dlshook.h"

#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>



extern "C" int chdir(const char *path) {
    return dlstools::DLSHook::chdir(path);
}

extern "C" int tcgetattr(int fd, struct termios *termios_p) {
    return dlstools::DLSHook::tcgetattr(fd, termios_p);
}

extern "C" int tcsetattr(int fd, int optional_actions,
                         const struct termios *termios_p) {
    return dlstools::DLSHook::tcsetattr(fd, optional_actions, termios_p);
}

extern "C" int tcflush(int fd, int queue_selector) {
    return dlstools::DLSHook::tcflush(fd, queue_selector);
}

extern "C" int tcdrain(int fd) {
    return dlstools::DLSHook::tcdrain(fd);
}

extern "C" int open(const char *path, int flags) {
    return dlstools::DLSHook::open(path, flags);
}

extern "C" int close(int fd) {
    return dlstools::DLSHook::close(fd);
}

extern "C" int write(int fd, const void *buf, size_t count) {
    return dlstools::DLSHook::write(fd, buf, count);
}

extern "C" int read(int fd, void *buf, size_t count) {
    return dlstools::DLSHook::read(fd, buf, count);
}

extern "C" FILE *fopen(const char *c_path, const char *mode) {
    return dlstools::DLSHook::fopen(c_path, mode);
}

extern "C" uint32_t DogConvert(void) {
    return dlstools::DLSHook::DogConvert();
}

void unprotect(void *addr, size_t size) {
  // unprotect the memory region
  mprotect((void *)((int)addr - ((int)addr % size)), size,
           PROT_READ | PROT_WRITE | PROT_EXEC);
}

void detour_function(void *new_adr, void *addr) {
  int call = (int)((int)new_adr - (int)addr - 5);
  unprotect(addr, 4096);
  *((unsigned char *)(addr)) = 0xE9;
  *((int *)((int)addr + 1)) = call;
}

static void __attribute__((destructor)) cleanup(void) {

}

void __attribute__((constructor)) initialize(void) {

}
