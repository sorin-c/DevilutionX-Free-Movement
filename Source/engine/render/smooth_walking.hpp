/**
 * @file smooth_walking.hpp
 *
 * Visual smoothing of walking players, so that walking along a line of tiles looks like walking in a straight line.
 * Only the rendering is affected, the players' positions in the game stay the same.
 */
#pragma once

#include <optional>

#include "engine/direction.hpp"
#include "engine/displacement.hpp"

namespace devilution {

struct Player;

/**
 * @brief Updates the smoothing of all walking players. Called once per rendered frame, before drawing.
 */
void UpdateSmoothWalking();

/**
 * @brief Screen space offset to add to where the player is drawn while walking.
 */
Displacement GetSmoothWalkingOffset(const Player &player);

/**
 * @brief Direction the player's walking sprite faces, when it differs from the direction of the current step.
 */
std::optional<Direction> GetSmoothWalkingFacing(const Player &player);

} // namespace devilution
