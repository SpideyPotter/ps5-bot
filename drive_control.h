#pragma once

struct MotorCommand { int left; int right; };

inline MotorCommand mixDrive(int forward, int reverse, int stickX,
                             bool boost, bool inverted) {
  const int triggerDeadzone = 10;
  const int stickDeadzone = 14;
  // Map each half separately so center and both extremes are exact.
  int turn = 0;
  if (stickX > stickDeadzone)
    turn = (stickX - stickDeadzone) * 100 / (127 - stickDeadzone);
  else if (stickX < -stickDeadzone)
    turn = (stickX + stickDeadzone) * 100 / (128 - stickDeadzone);

  MotorCommand result = {0, 0};
  if (boost) {
    result = {255, 255}; // Preserve R1's full-forward override.
  } else if (forward > triggerDeadzone || reverse > triggerDeadzone) {
    // Preserve R2 priority if both triggers are pressed.
    int speed = forward > triggerDeadzone ? forward : -reverse;
    result = {speed, speed};
    if (turn > 0) result.right = speed * (100 - turn) / 100;
    if (turn < 0) result.left = speed * (100 + turn) / 100;
  } else {
    // Pivot: opposite wheel directions, proportional to stick deflection.
    result.left = turn * 255 / 100;
    result.right = -result.left;
  }
  // A 180-degree front change reverses travel AND swaps wheel roles.
  // This preserves left/right steering relative to the selected front.
  if (inverted) return {-result.right, -result.left};
  return result;
}
