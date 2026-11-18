#pragma once
// Elero utility functions — extracted as pure functions for testability.

#include <cstddef>
#include <cstdint>
#include <string>

namespace esphome {
namespace elero {
namespace utils {

static constexpr uint8_t RSSI_SIGN_BIT = 127;
static constexpr int8_t RSSI_OFFSET = -74;
static constexpr int RSSI_DIVISOR = 2;

inline float calculate_rssi(uint8_t rssi_raw) {
  if (rssi_raw > RSSI_SIGN_BIT) {
    return static_cast<float>(static_cast<int8_t>(rssi_raw)) / RSSI_DIVISOR + RSSI_OFFSET;
  }
  return static_cast<float>(rssi_raw) / RSSI_DIVISOR + RSSI_OFFSET;
}

inline float registers_to_mhz(uint8_t freq2, uint8_t freq1, uint8_t freq0) {
  return (26.0f / 65536.0f) * ((static_cast<uint32_t>(freq2) << 16) |
                                (static_cast<uint32_t>(freq1) << 8) |
                                 static_cast<uint32_t>(freq0));
}

// Hex dump "AA.BB.CC (3)" for log output. Local replacement for ESPHome's
// format_hex_pretty(), which is no longer available in ESPHome 2026.11.
inline std::string hex_pretty(const uint8_t *data, size_t length) {
  static const char HEX_DIGITS[] = "0123456789ABCDEF";
  if (data == nullptr || length == 0)
    return "";
  std::string out;
  out.reserve(length * 3 + 8);
  for (size_t i = 0; i < length; i++) {
    if (i != 0)
      out.push_back('.');
    out.push_back(HEX_DIGITS[(data[i] >> 4) & 0x0F]);
    out.push_back(HEX_DIGITS[data[i] & 0x0F]);
  }
  out += " (" + std::to_string(length) + ")";
  return out;
}

}  // namespace utils
}  // namespace elero
}  // namespace esphome
