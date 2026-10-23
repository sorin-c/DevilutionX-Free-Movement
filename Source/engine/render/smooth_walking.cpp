/**
 * @file smooth_walking.cpp
 *
 * Implementation of the visual smoothing of walking players.
 */
#include "engine/render/smooth_walking.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "engine/animationinfo.h"
#include "engine/point.hpp"
#include "multi.h"
#include "options.h"
#include "player.h"
#include "utils/timer.hpp"

namespace devilution {

namespace {

struct Vector {
	float x = 0;
	float y = 0;

	Vector operator+(Vector other) const { return { x + other.x, y + other.y }; }
	Vector operator-(Vector other) const { return { x - other.x, y - other.y }; }
	Vector operator*(float factor) const { return { x * factor, y * factor }; }
};

struct SmoothWalkingState {
	bool walking = false;
	/** The tile the step being smoothed leads to. */
	Point future;
	Direction currentStep = Direction::South;
	/** The step taken right before the current one, if the player didn't stop in between. */
	std::optional<Direction> previousStep;
	Vector offset;
	Direction facing = Direction::South;
	uint32_t lastUpdate = 0;
};

std::array<SmoothWalkingState, MAX_PLRS> States;

/** How quickly the drawn position catches up when the path ahead changes, in milliseconds. */
constexpr float CatchUpTime = 60.F;

/** Facing is only changed when another direction matches the movement better by this much, to avoid flickering. */
constexpr float FacingHysteresis = 0.05F;

Vector GetStepVector(Direction dir)
{
	// Same as the offsets used by GetOffsetForWalking
	constexpr Vector StepVectors[8] = { { 0, 32 }, { -32, 16 }, { -64, 0 }, { -32, -16 }, { 0, -32 }, { 32, -16 }, { 64, 0 }, { 32, 16 } };
	return StepVectors[static_cast<size_t>(dir)];
}

std::optional<Direction> GetNextStep(const Player &player)
{
	switch (player.walkpath[0]) {
	case WALK_N: return Direction::North;
	case WALK_NE: return Direction::NorthEast;
	case WALK_E: return Direction::East;
	case WALK_SE: return Direction::SouthEast;
	case WALK_S: return Direction::South;
	case WALK_SW: return Direction::SouthWest;
	case WALK_W: return Direction::West;
	case WALK_NW: return Direction::NorthWest;
	default: return std::nullopt;
	}
}

/**
 * @brief Walking positions, relative to the start of the current step, along the previous, current and next steps.
 *
 * The current step goes from 0 to 1, the previous step from -1 to 0 and the next step from 1 to 2.
 */
struct WalkingLine {
	Vector previous;
	Vector current;
	Vector next;

	Vector at(float progress) const
	{
		if (progress < 0)
			return previous * progress;
		if (progress <= 1)
			return current * progress;
		return current + next * (progress - 1);
	}

	/** @brief Integral of at() from `from` to `to`. */
	Vector integrate(float from, float to) const
	{
		Vector sum;
		const auto squares = [](float a, float b) { return ((b * b) - (a * a)) / 2; };
		if (from < 0) {
			const float end = std::min(to, 0.F);
			sum = sum + previous * squares(from, end);
		}
		if (to > 0 && from < 1) {
			sum = sum + current * squares(std::max(from, 0.F), std::min(to, 1.F));
		}
		if (to > 1) {
			const float start = std::max(from, 1.F);
			sum = sum + current * (to - start) + next * squares(start - 1, to - 1);
		}
		return sum;
	}
};

Direction GetClosestFacing(Vector velocity, Direction currentFacing)
{
	const float length = std::sqrt((velocity.x * velocity.x) + (velocity.y * velocity.y));
	if (length < 0.001F)
		return currentFacing;

	const auto similarity = [&](Direction dir) {
		const Vector step = GetStepVector(dir);
		const float stepLength = std::sqrt((step.x * step.x) + (step.y * step.y));
		return ((step.x * velocity.x) + (step.y * velocity.y)) / (stepLength * length);
	};

	Direction best = currentFacing;
	float bestSimilarity = similarity(currentFacing) + FacingHysteresis;
	for (int i = 0; i < 8; i++) {
		const auto dir = static_cast<Direction>(i);
		const float dirSimilarity = similarity(dir);
		if (dirSimilarity > bestSimilarity) {
			best = dir;
			bestSimilarity = dirSimilarity;
		}
	}
	return best;
}

void UpdatePlayer(const Player &player, SmoothWalkingState &state, uint32_t now)
{
	if (!*GetOptions().Gameplay.smoothWalking || !player.plractive || !player.isOnActiveLevel() || !player.isWalking()) {
		state = {};
		return;
	}

	if (!state.walking || state.future != player.position.future) {
		// A new step started. It continues the previous one when it starts where the previous one ended.
		if (state.walking && player.position.tile == state.future)
			state.previousStep = state.currentStep;
		else
			state.previousStep = std::nullopt;
		if (!state.walking)
			state.facing = player._pdir;
		state.walking = true;
		state.future = player.position.future;
		state.currentStep = player._pdir;
	}

	const std::optional<Direction> nextStep = GetNextStep(player);
	const WalkingLine line {
		state.previousStep ? GetStepVector(*state.previousStep) : Vector {},
		GetStepVector(state.currentStep),
		nextStep ? GetStepVector(*nextStep) : Vector {},
	};

	// Average the position over a window of one step before and after the current position. This evens out the
	// corners between steps in different directions. Near the start and the end of a walk the window shrinks, so
	// the player is drawn exactly on the tile when standing.
	const float progress = static_cast<float>(player.AnimInfo.getAnimationProgress()) / AnimationInfo::baseValueFraction;
	const float earliest = state.previousStep ? -1.F : 0.F;
	const float latest = nextStep ? 2.F : 1.F;
	const float window = std::max(std::min({ 1.F, progress - earliest, latest - progress }), 0.F);

	Vector targetOffset;
	Vector velocity = line.current;
	if (window > 0.01F) {
		const Vector average = line.integrate(progress - window, progress + window) * (1 / (2 * window));
		targetOffset = average - line.at(progress);
		velocity = (line.at(progress + window) - line.at(progress - window)) * (1 / (2 * window));
	}

	const float elapsed = state.lastUpdate == 0 ? CatchUpTime : static_cast<float>(now - state.lastUpdate);
	const float catchUp = 1 - std::exp(-elapsed / CatchUpTime);
	state.offset = state.offset + (targetOffset - state.offset) * catchUp;
	state.lastUpdate = now;

	state.facing = GetClosestFacing(velocity, state.facing);
}

} // namespace

void UpdateSmoothWalking()
{
	const uint32_t now = std::max<uint32_t>(GetMillisecondsSinceStartup(), 1);
	for (size_t i = 0; i < States.size(); i++) {
		if (i < Players.size())
			UpdatePlayer(Players[i], States[i], now);
		else
			States[i] = {};
	}
}

Displacement GetSmoothWalkingOffset(const Player &player)
{
	const size_t id = player.getId();
	if (id >= States.size() || !States[id].walking || !player.isWalking())
		return {};
	const Vector offset = States[id].offset;
	return { static_cast<int>(std::lround(offset.x)), static_cast<int>(std::lround(offset.y)) };
}

std::optional<Direction> GetSmoothWalkingFacing(const Player &player)
{
	const size_t id = player.getId();
	if (id >= States.size() || !States[id].walking || !player.isWalking() || States[id].facing == player._pdir)
		return std::nullopt;
	return States[id].facing;
}

} // namespace devilution
