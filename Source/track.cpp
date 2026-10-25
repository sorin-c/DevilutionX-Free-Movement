/**
 * @file track.cpp
 *
 * Implementation of functionality tracking what the mouse cursor is pointing at.
 */
#include "track.h"

#include <cstdint>
#include <optional>

#include "controls/control_mode.hpp"
#include "controls/game_controls.h"
#include "controls/plrctrls.h"
#include "cursor.h"
#include "diablo.h"
#include "engine/point.hpp"
#include "engine/render/scrollrt.h"
#include "levels/dun_tile_data.hpp"
#include "monster.h"
#include "msg.h"
#include "objects.h"
#include "options.h"
#include "player.h"
#include "stores.h"

namespace devilution {

namespace {

/**
 * @brief A repeated request that needs the player to walk, together with everything the path search for it depends on.
 */
struct WalkingRequest {
	PlayerActionType action;
	/** Tile the path search starts from. */
	Point start;
	/** Tile the player needs to walk to. */
	Point destination;
	uint32_t walkabilityGeneration;
	/** Players without life don't block their tile. This isn't covered by the generation, because a player's life is changed in too many places. */
	uint32_t lifelessPlayers;

	bool operator==(const WalkingRequest &other) const = default;
};

std::optional<WalkingRequest> LastWalkingRequest;

uint32_t GetLifelessPlayersMask()
{
	uint32_t mask = 0;
	for (const Player &player : Players) {
		if (player.hasNoLife())
			mask |= 1U << player.getId();
	}
	return mask;
}

/**
 * @brief Checks whether repeating a request that needs the player to walk to the given tile is pointless.
 *
 * A request that finds a path makes the player leave their tile. So if the same request was already made from
 * the same tile and nothing that affects path finding has changed since, the path search failed and would fail
 * again.
 */
bool IsWalkingRequestRedundant(const Player &player, Point destination)
{
	const WalkingRequest request { LastPlayerAction, player.position.future, destination, WalkabilityGeneration, GetLifelessPlayersMask() };
	if (LastWalkingRequest == request)
		return true;

	LastWalkingRequest = request;
	return false;
}

/**
 * @brief Checks whether repeating a melee attack on a target at the given tile is pointless, @see IsWalkingRequestRedundant
 */
bool IsMeleeAttackRedundant(const Player &player, Point targetPosition)
{
	// The target is in reach, so the attack doesn't depend on path finding.
	if (player.position.future.WalkingDistance(targetPosition) <= 1)
		return false;

	return IsWalkingRequestRedundant(player, targetPosition);
}

void RepeatWalk(Player &player)
{
	if (*GetOptions().Gameplay.freeMovement) {
		FreeWalkToMouse();
		return;
	}

	if (!InDungeonBounds(cursPosition))
		return;

	// Update the destination while a step is still in progress, so the next step is already queued when the current
	// one ends. Waiting for the last frames of the step makes the command arrive too late and the player stands still
	// for a moment between steps.
	if (player._pmode != PM_STAND && !player.isWalking())
		return;

	const Point destination = GetWalkDestination(player, cursPosition);
	if (destination == player.GetTargetPosition())
		return;

	if (IsWalkingRequestRedundant(player, destination))
		return;

	NetSendCmdLoc(MyPlayerId, true, CMD_WALKXY, destination);
}

} // namespace

void FreeWalkToMouse()
{
	const Player &player = *MyPlayer;

	// Where the player stands on the screen: the center of their tile, moved by any walking or free movement
	Point playerOnScreen = GetScreenPosition(player.position.tile) + Displacement { TILE_WIDTH / 2, -TILE_HEIGHT / 2 } + GetFreeMovementRenderingOffset(player);
	if (player.isWalking())
		playerOnScreen += GetOffsetForWalking(player.AnimInfo, player._pdir);
	Displacement toMouse = MousePosition - playerOnScreen;
	if (*GetOptions().Graphics.zoom)
		toMouse = Point { MousePosition.x / 2, MousePosition.y / 2 } - playerOnScreen;

	// Invert the projection of tiles to the screen: one tile along x is { 32, 16 } on screen, one tile along y { -32, 16 }
	const DisplacementOf<int32_t> worldOffset {
		(toMouse.deltaX * FreeMovementUnitsPerTile / TILE_WIDTH) + (toMouse.deltaY * FreeMovementUnitsPerTile / TILE_HEIGHT),
		(toMouse.deltaY * FreeMovementUnitsPerTile / TILE_HEIGHT) - (toMouse.deltaX * FreeMovementUnitsPerTile / TILE_WIDTH),
	};
	PointOf<int32_t> target = GetFreePosition(player) + worldOffset;
	constexpr int32_t MaxCoordinate = (MAXDUNX - 1) * FreeMovementUnitsPerTile;
	target = { std::clamp(target.x, 0, MaxCoordinate), std::clamp(target.y, 0, MaxCoordinate) };

	if (player.freeMoving && player.freeTarget == target)
		return;
	NetSendCmdFreeMove(target);
}

Point GetWalkDestination(const Player &player, Point clickedTile)
{
	const Point start = player.position.future;
	const Displacement delta = clickedTile - start;
	const int length = start.WalkingDistance(clickedTile);
	// Go back along the line from the clicked tile towards the player until a tile can be walked on.
	for (int i = length; i > 0; i--) {
		const Point tile = start + Displacement { delta.deltaX * i / length, delta.deltaY * i / length };
		if (PosOkPlayer(player, tile))
			return tile;
	}
	return clickedTile;
}

void InvalidateTargets()
{
	if (pcursmonst != -1) {
		const Monster &monster = Monsters[pcursmonst];
		if (monster.isInvalid || monster.hasNoLife()
		    || (monster.flags & MFLAG_HIDDEN) != 0
		    || !IsTileLit(monster.position.tile)) {
			pcursmonst = -1;
		}
	}

	if (ObjectUnderCursor != nullptr && !ObjectUnderCursor->canInteractWith())
		ObjectUnderCursor = nullptr;

	if (PlayerUnderCursor != nullptr) {
		const Player &targetPlayer = *PlayerUnderCursor;
		if (targetPlayer._pmode == PM_DEATH || targetPlayer._pmode == PM_QUIT || !targetPlayer.plractive
		    || !targetPlayer.isOnActiveLevel() || targetPlayer.hasNoLife()
		    || !IsTileLit(targetPlayer.position.tile))
			PlayerUnderCursor = nullptr;
	}
}

void RepeatPlayerAction()
{
	if (pcurs != CURSOR_HAND)
		return;

	if (sgbMouseDown == CLICK_NONE && ControllerActionHeld == GameActionType_NONE)
		return;

	if (IsPlayerInStore())
		return;

	if (LastPlayerAction == PlayerActionType::None)
		return;

	Player &myPlayer = *MyPlayer;
	if (myPlayer.destAction != ACTION_NONE)
		return;
	if (myPlayer._pInvincible)
		return;
	// Walking can change its destination during a step, see RepeatWalk.
	if (LastPlayerAction != PlayerActionType::Walk && !myPlayer.CanChangeAction())
		return;

	const bool rangedAttack = myPlayer.UsesRangedWeapon();
	switch (LastPlayerAction) {
	case PlayerActionType::Attack:
		if (InDungeonBounds(cursPosition))
			NetSendCmdLoc(MyPlayerId, true, rangedAttack ? CMD_RATTACKXY : CMD_SATTACKXY, cursPosition);
		break;
	case PlayerActionType::AttackMonsterTarget:
		if (pcursmonst != -1) {
			if (!rangedAttack && IsMeleeAttackRedundant(myPlayer, Monsters[pcursmonst].position.future))
				break;
			NetSendCmdParam1(true, rangedAttack ? CMD_RATTACKID : CMD_ATTACKID, pcursmonst);
		}
		break;
	case PlayerActionType::AttackPlayerTarget:
		if (PlayerUnderCursor != nullptr && !myPlayer.friendlyMode) {
			if (!rangedAttack && IsMeleeAttackRedundant(myPlayer, PlayerUnderCursor->position.future))
				break;
			NetSendCmdParam1(true, rangedAttack ? CMD_RATTACKPID : CMD_ATTACKPID, PlayerUnderCursor->getId());
		}
		break;
	case PlayerActionType::Spell:
		if (ControlMode != ControlTypes::KeyboardAndMouse) {
			UpdateSpellTarget(MyPlayer->_pRSpell);
		}
		CheckPlrSpell(ControlMode == ControlTypes::KeyboardAndMouse);
		break;
	case PlayerActionType::SpellMonsterTarget:
		if (pcursmonst != -1)
			CheckPlrSpell(false);
		break;
	case PlayerActionType::SpellPlayerTarget:
		if (PlayerUnderCursor != nullptr && !myPlayer.friendlyMode)
			CheckPlrSpell(false);
		break;
	case PlayerActionType::OperateObject:
		if (ObjectUnderCursor != nullptr && !ObjectUnderCursor->isDoor()) {
			if (!IsPlayerAdjacentToObject(myPlayer, *ObjectUnderCursor) && IsWalkingRequestRedundant(myPlayer, cursPosition))
				break;
			NetSendCmdLoc(MyPlayerId, true, CMD_OPOBJXY, cursPosition);
		}
		break;
	case PlayerActionType::Walk:
		RepeatWalk(myPlayer);
		break;
	case PlayerActionType::None:
		break;
	}
}

bool track_isscrolling()
{
	return LastPlayerAction == PlayerActionType::Walk;
}

} // namespace devilution
