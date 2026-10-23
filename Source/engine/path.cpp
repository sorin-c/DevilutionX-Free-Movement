/**
 * @file path.cpp
 *
 * Implementation of the path finding algorithms.
 */
#include "engine/path.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

#include <function_ref.hpp>

#include "appfat.h"
#include "crawl.hpp"
#include "engine/displacement.hpp"
#include "engine/point.hpp"
#include "utils/algorithm/container.hpp"
#include "utils/static_vector.hpp"

namespace devilution {

// The frame times for axis-aligned and diagonal steps are actually the same.
//
// However, we set the diagonal step cost a bit higher to avoid
// excessive diagonal movement. For example, the frame times for these
// two paths are the same: ↑↑ and ↗↖. However, ↑↑ looks more natural.
const int PathAxisAlignedStepCost = 100;
const int PathDiagonalStepCost = 101;

namespace {

constexpr size_t MaxPathNodes = 1024;

using NodeIndexType = uint16_t;
using CoordType = uint8_t;
using CostType = uint16_t;
using PointT = PointOf<CoordType>;

struct FrontierNode {
	PointT position;

	// Current best guess of the cost of the path to destination
	// if it goes through this node.
	CostType f;
};

struct ExploredNode {
	// Preceding node (needed to reconstruct the path at the end).
	PointT prev;

	// The current lowest cost from start to this node (0 for the start node).
	CostType g;
};

// A simple map with a fixed number of buckets and static storage.
class ExploredNodes {
	static const size_t NumBuckets = 64;
	static const size_t BucketCapacity = 3 * MaxPathNodes / NumBuckets;
	using Entry = std::pair<uint16_t, ExploredNode>;
	using Bucket = StaticVector<Entry, BucketCapacity>;

public:
	using value_type = Entry;
	using iterator = value_type *;
	using const_iterator = const value_type *;

	[[nodiscard]] const_iterator find(const PointT &point) const
	{
		const Bucket &b = bucket(point);
		const auto *const it = c_find_if(b, [r = repr(point)](const Entry &e) { return e.first == r; });
		if (it == b.end()) return nullptr;
		return it;
	}
	[[nodiscard]] iterator find(const PointT &point)
	{
		Bucket &b = bucket(point);
		auto *it = c_find_if(b, [r = repr(point)](const Entry &e) { return e.first == r; });
		if (it == b.end()) return nullptr;
		return it;
	}

	// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
	[[nodiscard]] const_iterator end() const { return nullptr; }
	// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
	[[nodiscard]] iterator end() { return nullptr; }

	void emplace(const PointT &point, const ExploredNode &exploredNode)
	{
		bucket(point).emplace_back(repr(point), exploredNode);
	}

	[[nodiscard]] bool canInsert(const PointT &point) const
	{
		return bucket(point).size() < BucketCapacity;
	}

private:
	[[nodiscard]] const Bucket &bucket(const PointT &point) const { return buckets_[bucketIndex(point)]; }
	[[nodiscard]] Bucket &bucket(const PointT &point) { return buckets_[bucketIndex(point)]; }
	[[nodiscard]] static size_t bucketIndex(const PointT &point)
	{
		return ((point.x & 0b111) << 3) | (point.y & 0b111);
	}

	[[nodiscard]] static uint16_t repr(const PointT &point)
	{
		return (point.x << 8) | point.y;
	}

	std::array<Bucket, NumBuckets> buckets_;
};

bool IsDiagonalStep(const Point &a, const Point &b)
{
	return a.x != b.x && a.y != b.y;
}

/**
 * @brief Returns the distance between 2 adjacent nodes.
 */
CostType GetDistance(PointT startPosition, PointT destinationPosition)
{
	return IsDiagonalStep(startPosition, destinationPosition)
	    ? PathDiagonalStepCost
	    : PathAxisAlignedStepCost;
}

/**
 * @brief heuristic, estimated cost from startPosition to destinationPosition.
 */
CostType GetHeuristicCost(PointT startPosition, PointT destinationPosition)
{
	// This function needs to be admissible, i.e. it should never over-estimate
	// the distance.
	//
	// This calculation assumes we can take diagonal steps until we reach
	// the same row or column and then take the remaining axis-aligned steps.
	const int dx = std::abs(static_cast<int>(startPosition.x) - static_cast<int>(destinationPosition.x));
	const int dy = std::abs(static_cast<int>(startPosition.y) - static_cast<int>(destinationPosition.y));
	const int diagSteps = std::min(dx, dy);

	// After we've taken `diagSteps`, the remaining steps in one coordinate
	// will be zero, and in the other coordinate it will be reduced by `diagSteps`.
	// We then still need to take the remaining steps:
	//   max(dx, dy) - diagSteps = max(dx, dy) - min(dx, dy) = abs(dx - dy)
	const int axisAlignedSteps = std::abs(dx - dy);
	return (diagSteps * PathDiagonalStepCost) + (axisAlignedSteps * PathAxisAlignedStepCost);
}

int ReconstructPath(const ExploredNodes &explored, PointT dest, int8_t *path, size_t maxPathLength)
{
	size_t len = 0;
	PointT cur = dest;
	while (true) {
		const auto *const it = explored.find(cur);
		if (it == explored.end()) app_fatal("Failed to reconstruct path");
		if (it->second.g == 0) break; // reached start
		if (len == maxPathLength) {
			// Path too long.
			len = 0;
			break;
		}
		path[len++] = GetPathDirection(it->second.prev, cur);
		cur = it->second.prev;
	}
	std::reverse(path, path + len);
	std::fill(path + len, path + maxPathLength, -1);
	return static_cast<int>(len);
}

} // namespace

int8_t GetPathDirection(Point startPosition, Point destinationPosition)
{
	constexpr int8_t PathDirections[9] = { 5, 1, 6, 2, 0, 3, 8, 4, 7 };
	return PathDirections[(3 * (destinationPosition.y - startPosition.y)) + 4 + destinationPosition.x - startPosition.x];
}

Displacement GetPathStepDisplacement(int8_t step)
{
	// Inverse of GetPathDirection
	constexpr Displacement StepDisplacements[9] = { { 0, 0 }, { 0, -1 }, { -1, 0 }, { 1, 0 }, { 0, 1 }, { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } };
	return StepDisplacements[step];
}

int FindPath(tl::function_ref<bool(Point, Point)> canStep, tl::function_ref<bool(Point)> posOk, Point startPosition, Point destinationPosition, int8_t *path, size_t maxPathLength)
{
	const PointT start { startPosition };
	const PointT dest { destinationPosition };

	const CostType initialHeuristicCost = GetHeuristicCost(start, dest);
	if (initialHeuristicCost > PathDiagonalStepCost * maxPathLength) {
		// Heuristic cost never underestimates the true cost, so we can give up early.
		return 0;
	}

	StaticVector<FrontierNode, MaxPathNodes> frontier;
	ExploredNodes explored;
	{
		frontier.emplace_back(FrontierNode { .position = start, .f = initialHeuristicCost });
		explored.emplace(start, ExploredNode { .prev = {}, .g = 0 });
	}

	const auto frontierComparator = [&explored, &dest](const FrontierNode &a, const FrontierNode &b) {
		// We use heap functions from <algorithm> which form a max-heap.
		// We reverse the comparison sign here to get a min-heap.
		if (a.f != b.f) return a.f > b.f;

		// For nodes with the same f-score, prefer the ones with lower
		// heuristic cost (likely to be closer to the goal).
		const CostType hA = GetHeuristicCost(a.position, dest);
		const CostType hB = GetHeuristicCost(b.position, dest);
		if (hA != hB) return hA > hB;

		// Prefer diagonal steps first.
		const ExploredNode &aInfo = explored.find(a.position)->second;
		const ExploredNode &bInfo = explored.find(b.position)->second;
		const bool isDiagonalA = IsDiagonalStep(aInfo.prev, a.position);
		const bool isDiagonalB = IsDiagonalStep(bInfo.prev, b.position);
		if (isDiagonalA != isDiagonalB) return isDiagonalB;

		// Finally, disambiguate by coordinate:
		if (a.position.x != b.position.x) return a.position.x > b.position.x;
		return a.position.y > b.position.y;
	};

	while (!frontier.empty()) {
		const FrontierNode cur = frontier.front(); // argmin(node.f) for node in openSet

		if (cur.position == destinationPosition) {
			return ReconstructPath(explored, cur.position, path, maxPathLength);
		}

		std::pop_heap(frontier.begin(), frontier.end(), frontierComparator);
		frontier.pop_back();
		const CostType curG = explored.find(cur.position)->second.g;

		// Discard invalid nodes.

		// If this node is already at the maximum number of steps, we can skip processing it.
		// We don't keep track of the maximum number of steps, so we approximate it.
		if (curG >= PathDiagonalStepCost * maxPathLength) continue;

		// When we discover a better path to a node, we push the node to the heap
		// with the new `f` value even if the node is already in the heap.
		if (curG + GetHeuristicCost(cur.position, dest) > cur.f) continue;

		for (const DisplacementOf<int8_t> d : PathDirs) {
			// We're using `uint8_t` for coordinates. Avoid underflow:
			if ((cur.position.x == 0 && d.deltaX < 0) || (cur.position.y == 0 && d.deltaY < 0)) continue;
			const PointT neighborPos = cur.position + d;
			const bool ok = posOk(neighborPos);
			if (ok) {
				if (!canStep(cur.position, neighborPos)) continue;
			} else {
				// We allow targeting a non-walkable node if it is the destination.
				if (neighborPos != dest) continue;
			}
			const CostType g = curG + GetDistance(cur.position, neighborPos);
			if (curG >= PathDiagonalStepCost * maxPathLength) continue;
			bool improved = false;
			if (auto *it = explored.find(neighborPos); it == explored.end()) {
				if (explored.canInsert(neighborPos)) {
					explored.emplace(neighborPos, ExploredNode { .prev = cur.position, .g = g });
					improved = true;
				}
			} else if (it->second.g > g) {
				it->second.prev = cur.position;
				it->second.g = g;
				improved = true;
			}
			if (improved) {
				const CostType f = g + GetHeuristicCost(neighborPos, dest);
				if (frontier.size() < MaxPathNodes) {
					// We always push the node to the heap, even if the same position already exists in it.
					// When popping from the heap, we discard invalid nodes by checking that `g + h <= f`.
					frontier.emplace_back(FrontierNode { .position = neighborPos, .f = f });
					std::push_heap(frontier.begin(), frontier.end(), frontierComparator);
				}
			}
		}
	}

	return 0; // no path
}

namespace {

/** @brief Longest part of a path that StraightenPath replaces with a single straight line. */
constexpr int MaxStraightSegmentLength = 24;

/**
 * @brief Divides and rounds half away from zero, so lines are symmetric in all directions.
 */
int DivideRounded(int numerator, int denominator)
{
	const int rounded = ((2 * std::abs(numerator)) + denominator) / (2 * denominator);
	return numerator < 0 ? -rounded : rounded;
}

/**
 * @brief Writes the steps along the tiles closest to the straight line between two positions.
 * @return false if the line can't be walked
 */
bool TryGetStraightSteps(tl::function_ref<bool(Point, Point)> canStep, tl::function_ref<bool(Point)> posOk, Point from, Point to, Point pathDestination, int8_t *steps)
{
	const Displacement delta = to - from;
	const int length = std::max(std::abs(delta.deltaX), std::abs(delta.deltaY));
	Point previous = from;
	for (int i = 1; i <= length; i++) {
		const Point next = from + Displacement { DivideRounded(delta.deltaX * i, length), DivideRounded(delta.deltaY * i, length) };
		if (posOk(next)) {
			if (!canStep(previous, next))
				return false;
		} else if (next != pathDestination) {
			// Like FindPath, only the destination of the path may be a tile that can't be walked on.
			return false;
		}
		steps[i - 1] = GetPathDirection(previous, next);
		previous = next;
	}
	return true;
}

} // namespace

void StraightenPath(tl::function_ref<bool(Point, Point)> canStep, tl::function_ref<bool(Point)> posOk, Point startPosition, int8_t *path, int pathLength)
{
	if (pathLength < 2 || pathLength > static_cast<int>(MaxPathLengthPlayer))
		return;

	Point positions[MaxPathLengthPlayer + 1];
	positions[0] = startPosition;
	for (int i = 0; i < pathLength; i++)
		positions[i + 1] = positions[i] + GetPathStepDisplacement(path[i]);
	const Point destination = positions[pathLength];

	int8_t steps[MaxStraightSegmentLength];
	int start = 0;
	while (start < pathLength - 1) {
		int end = std::min(pathLength, start + MaxStraightSegmentLength);
		for (; end > start + 1; end--) {
			// The line needs as many steps as the part of the path it replaces, so the path doesn't get longer.
			if (positions[start].WalkingDistance(positions[end]) != end - start)
				continue;
			if (TryGetStraightSteps(canStep, posOk, positions[start], positions[end], destination, steps))
				break;
		}
		if (end > start + 1) {
			std::copy(steps, steps + (end - start), path + start);
			start = end;
		} else {
			start++;
		}
	}
}

std::optional<Point> FindClosestValidPosition(tl::function_ref<bool(Point)> posOk, Point startingPosition, unsigned int minimumRadius, unsigned int maximumRadius)
{
	return Crawl(minimumRadius, maximumRadius, [&](Displacement displacement) -> std::optional<Point> {
		Point candidatePosition = startingPosition + displacement;
		if (posOk(candidatePosition))
			return candidatePosition;
		return {};
	});
}

#ifdef BUILD_TESTING
int TestPathGetHeuristicCost(Point startPosition, Point destinationPosition)
{
	return GetHeuristicCost(startPosition, destinationPosition);
}
#endif

} // namespace devilution
