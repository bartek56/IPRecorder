#ifndef ATCONFIG_HPP
#define ATCONFIG_HPP

#include <vector>
#include <string>

#if SIMULATOR == 1
static const std::vector<std::string> k_defaultConfig{"AT"};
static constexpr uint32_t k_waitForMessageTimeout = 10000;
static constexpr uint32_t k_waitForConfirmTimeout = 5000;
#endif

#if SIMULATOR == 0
static const std::vector<std::string> k_defaultConfig{"AT", "AT+CMGF=1", "AT+CNMI=1,2,0,0", "AT+CLIP=1"};
static constexpr uint32_t k_waitForMessageTimeout = 2000;
static constexpr uint32_t k_waitForConfirmTimeout = 500;
#endif

#endif
