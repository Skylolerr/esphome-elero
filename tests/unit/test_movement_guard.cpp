#include "elero/elero_movement_guard.h"
#include <gtest/gtest.h>

using namespace esphome::elero::movement_guard;

TEST(MovementGuard, StaleTopReplyContradictsClosing) {
  // Motor answers a CLOSE with its pre-movement state TOP.
  EXPECT_TRUE(contradicts(Direction::DOWN, STATE_TOP));
  EXPECT_TRUE(contradicts(Direction::DOWN, 0x0d));  // stopped
  EXPECT_TRUE(contradicts(Direction::DOWN, 0x03));  // intermediate
  EXPECT_TRUE(contradicts(Direction::DOWN, STATE_MOVING_UP));
  EXPECT_TRUE(contradicts(Direction::UP, STATE_BOTTOM));
}

TEST(MovementGuard, ConsistentStatesConfirm) {
  EXPECT_TRUE(confirms(Direction::DOWN, STATE_START_MOVING_DOWN));
  EXPECT_TRUE(confirms(Direction::DOWN, STATE_MOVING_DOWN));
  EXPECT_TRUE(confirms(Direction::DOWN, STATE_BOTTOM));
  EXPECT_TRUE(confirms(Direction::DOWN, STATE_BOTTOM_TILT));
  EXPECT_TRUE(confirms(Direction::UP, STATE_MOVING_UP));
  EXPECT_TRUE(confirms(Direction::UP, STATE_TOP_TILT));
  EXPECT_FALSE(contradicts(Direction::UP, STATE_TOP));
}

TEST(MovementGuard, FaultStatesAreNeverIgnored) {
  EXPECT_FALSE(contradicts(Direction::DOWN, STATE_BLOCKING));
  EXPECT_FALSE(contradicts(Direction::UP, STATE_OVERHEATED));
  EXPECT_FALSE(contradicts(Direction::UP, STATE_TIMEOUT));
}

TEST(MovementGuard, NoDirectionNoGuard) {
  EXPECT_FALSE(contradicts(Direction::NONE, STATE_TOP));
}

TEST(MovementGuard, WindowIncludingMillisWrap) {
  EXPECT_TRUE(guard_active(1000, 500, false, false));
  EXPECT_FALSE(guard_active(500 + GUARD_MS, 500, false, false));
  EXPECT_TRUE(guard_active(10, 0xFFFFFF00u, false, false));
  EXPECT_FALSE(guard_active(1000, 0, false, false));
  EXPECT_TRUE(guard_active(99999, 0, true, false));   // command still queued
  EXPECT_TRUE(guard_active(99999, 100, false, true));  // verification pending
}

TEST(MovementGuard, VerificationSequence) {
  EXPECT_EQ(next_verify_action(true, false, 0), VerifyAction::DONE);
  EXPECT_EQ(next_verify_action(false, false, 0), VerifyAction::POLL);
  EXPECT_EQ(next_verify_action(false, true, 0), VerifyAction::RESEND);
  EXPECT_EQ(next_verify_action(false, true, MAX_RESENDS - 1), VerifyAction::RESEND);
  EXPECT_EQ(next_verify_action(false, true, MAX_RESENDS), VerifyAction::GIVE_UP);
}
