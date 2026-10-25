/**
 * @file track.h
 *
 * Interface of functionality tracking what the mouse cursor is pointing at.
 */
#pragma once

#include "engine/point.hpp"

namespace devilution {

struct Player;

/**
 * @brief Picks the tile to walk to when the player clicks on the given tile.
 *
 * When the tile can't be walked on, for example because it is part of a house or a tree, the player walks to the
 * closest tile on the way there that can be walked on instead of not moving at all.
 */
Point GetWalkDestination(const Player &player, Point clickedTile);

/**
 * @brief Starts free movement (see StartFreeMovement) of the local player towards the point under the mouse cursor.
 */
void FreeWalkToMouse();
void InvalidateTargets();
void RepeatPlayerAction();
bool track_isscrolling();

} // namespace devilution
