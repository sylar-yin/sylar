#ifndef __SYLAR_UTIL_HASH_UTIL_H__
#define __SYLAR_UTIL_HASH_UTIL_H__

#include <stdint.h>
#include <string>
#include <vector>

namespace sylar {

uint32_t murmur3_hash(const char * str, const uint32_t & seed = 1060627423);
uint64_t murmur3_hash64(const char * str, const uint32_t & seed = 1060627423, const uint32_t& seed2 = 1050126127);
uint32_t murmur3_hash(const void* str, const uint32_t& size, const uint32_t & seed = 1060627423);
uint64_t murmur3_hash64(const void* str, const uint32_t& size,  const uint32_t & seed = 1060627423, const uint32_t& seed2 = 1050126127);
uint32_t quick_hash(const char * str);
uint32_t quick_hash(const void* str, uint32_t size);

std::string base64decode(const std::string &src);
std::string base64encode(const std::string &data);
std::string base64encode(const void *data, size_t len);

// Returns result in hex
std::string md5(const std::string &data);
std::string sha1(const std::string &data);
// Returns result in blob
std::string md5sum(const std::string &data);
std::string md5sum(const void *data, size_t len);
std::string sha0sum(const std::string &data);
std::string sha0sum(const void *data, size_t len);
std::string sha1sum(const std::string &data);
std::string sha1sum(const void *data, size_t len);
std::string hmac_md5(const std::string &text, const std::string &key);
std::string hmac_sha1(const std::string &text, const std::string &key);
std::string hmac_sha256(const std::string &text, const std::string &key);

/// Output must be of size len * 2, and will *not* be null-terminated
void hexstring_from_data(const void *data, size_t len, char *output);
std::string hexstring_from_data(const void *data, size_t len);
std::string hexstring_from_data(const std::string &data);

/// Output must be of size length / 2, and will *not* be null-terminated
/// std::invalid_argument will be thrown if hexstring is not hex
void data_from_hexstring(const char *hexstring, size_t length, void *output);
std::string data_from_hexstring(const char *hexstring, size_t length);
std::string data_from_hexstring(const std::string &data);

void replace(std::string &str, char find, char replaceWith);
void replace(std::string &str, char find, const std::string &replaceWith);
void replace(std::string &str, const std::string &find, const std::string &replaceWith);

std::vector<std::string> split(const std::string &str, char delim, size_t max = ~0);
std::vector<std::string> split(const std::string &str, const char *delims, size_t max = ~0);

std::string random_string(size_t len);
}

#endif
