#ifndef ATCONFIG_HPP
#define ATCONFIG_HPP

#include <vector>
#include <string>
#include <string_view>
#include <cstdint>


#if SIMULATOR == 1
static const std::vector<std::string> k_defaultConfig{"AT_CONFIG1", "AT_CONFIG2"};
static constexpr uint32_t k_waitForMessageTimeout = 15000;
static constexpr uint32_t k_waitForConfirmTimeout = 10000;
#endif

#if SIMULATOR == 0
static const std::vector<std::string> k_defaultConfig{"AT", "AT+CMGF=1", "AT+CNMI=1,2,0,0", "AT+CLIP=1"};
static constexpr uint32_t k_waitForMessageTimeout = 8000;
static constexpr uint32_t k_waitForConfirmTimeout = 2000;
#endif

// Requests
static constexpr std::string_view AT_SMS_REQUEST = "AT+CMGS";
static constexpr std::string_view SMS_SENDING_CONFIRM = "+CMGS";
static constexpr std::string_view AT_SYNC = "AT";

// Reponses
static constexpr std::string_view CALLING = "+CLIP:";
static constexpr std::string_view SMS_RESPONSE = "+CMT:";
static constexpr std::string_view ERROR = "ERROR";
static constexpr std::string_view RING = "RING";
static constexpr std::string_view OK = "OK\r";

static constexpr char SUB = 0x1A;
static constexpr std::string_view SMS_INPUT = ">";


#endif
