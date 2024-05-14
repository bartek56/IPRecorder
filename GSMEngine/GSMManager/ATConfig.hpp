#ifndef ATCONFIG_HPP
#define ATCONFIG_HPP

#include <vector>
#include <string>

static const std::vector<std::string> k_defaultConfig{"AT", "AT+CMGF=1", "AT+CNMI=1,2,0,0", "AT+CLIP=1"};
static constexpr uint32_t k_waitForMessageTimeout = 5000;
static constexpr uint32_t k_waitForConfirmTimeout = 500;

#endif
