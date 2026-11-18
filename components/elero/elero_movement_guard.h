#pragma once
// Movement guard and movement verification — pure functions for testability.
//
// Background (observed on real Elero receivers, incl. venetian-blind motors):
// a motor answers a movement command immediately with its *current* state,
// i.e. still TOP/STOPPED/INTERMEDIATE before it starts to move. Treating that
// reply as truth cancels the cover's movement tracking: the target-position
// STOP is never sent (the blind runs to its end stop) and Home Assistant shows
// a wrong position. Conversely, a lost packet is never noticed because the
// protocol has no ACK.
//
// Fix: for a short window after a movement command was transmitted, status
// frames that contradict the commanded direction are ignored. Shortly after
// the command, the cover verifies that the motor reports movement in the
// commanded direction (or the commanded end position). If not, the command is
// sent again a bounded number of times.

#include <cstdint>

namespace esphome {
namespace elero {
namespace movement_guard {

enum class Direction : uint8_t { NONE = 0, UP, DOWN };

// Elero state codes (mirrors elero.h; duplicated to keep this header standalone).
static constexpr uint8_t STATE_TOP = 0x01;
static constexpr uint8_t STATE_BOTTOM = 0x02;
static constexpr uint8_t STATE_BLOCKING = 0x05;
static constexpr uint8_t STATE_OVERHEATED = 0x06;
static constexpr uint8_t STATE_TIMEOUT = 0x07;
static constexpr uint8_t STATE_START_MOVING_UP = 0x08;
static constexpr uint8_t STATE_START_MOVING_DOWN = 0x09;
static constexpr uint8_t STATE_MOVING_UP = 0x0a;
static constexpr uint8_t STATE_MOVING_DOWN = 0x0b;
static constexpr uint8_t STATE_TOP_TILT = 0x0e;
static constexpr uint8_t STATE_BOTTOM_TILT = 0x0f;

// Window after the latest movement transmission in which contradicting status
// frames are ignored.
static constexpr uint32_t GUARD_MS = 3000;
// Delay after the movement transmission before verification starts.
static constexpr uint32_t VERIFY_DELAY_MS = 2500;
// Wait for the reply to the verification CHECK.
static constexpr uint32_t VERIFY_POLL_WAIT_MS = 1500;
// Additional transmissions of the same movement if it is not confirmed.
static constexpr uint8_t MAX_RESENDS = 2;

inline bool is_fault_state(uint8_t state) {
  return state == STATE_BLOCKING || state == STATE_OVERHEATED || state == STATE_TIMEOUT;
}

// True if the state is consistent with (= confirms) the commanded direction:
// moving that way, starting that way, or already at that end position.
inline bool confirms(Direction dir, uint8_t state) {
  switch (dir) {
    case Direction::UP:
      return state == STATE_START_MOVING_UP || state == STATE_MOVING_UP ||
             state == STATE_TOP || state == STATE_TOP_TILT;
    case Direction::DOWN:
      return state == STATE_START_MOVING_DOWN || state == STATE_MOVING_DOWN ||
             state == STATE_BOTTOM || state == STATE_BOTTOM_TILT;
    default:
      return true;
  }
}

// A state that must be ignored while the guard is active. Fault states are
// never ignored: the motor reports a real problem and no resend may follow.
inline bool contradicts(Direction dir, uint8_t state) {
  return dir != Direction::NONE && !is_fault_state(state) && !confirms(dir, state);
}

inline bool guard_active(uint32_t now, uint32_t command_ms, bool command_pending,
                         bool verification_pending) {
  if (command_pending || verification_pending)
    return true;
  return command_ms != 0 && (now - command_ms) < GUARD_MS;
}

enum class VerifyAction : uint8_t { NONE, DONE, POLL, RESEND, GIVE_UP };

// Decide the next verification step once the verification deadline is due.
inline VerifyAction next_verify_action(bool confirmed, bool polled, uint8_t resends) {
  if (confirmed)
    return VerifyAction::DONE;
  if (!polled)
    return VerifyAction::POLL;
  if (resends < MAX_RESENDS)
    return VerifyAction::RESEND;
  return VerifyAction::GIVE_UP;
}

}  // namespace movement_guard
}  // namespace elero
}  // namespace esphome
