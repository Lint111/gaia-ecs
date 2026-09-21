#include "test_common.h"

#define TestWorld SparseTestWorld

#if GAIA_OBSERVERS_ENABLED

namespace {

enum class ObserverTrace : uint32_t {
	TriggerEnter = 1,
	PairDelete = 2,
	PairAdd = 3,
	TriggerExit = 4,
};

void print_trace(const char* label, const cnt::darr<ObserverTrace>& trace) {
	std::printf("%s:", label);
	for (const auto item: trace)
		std::printf(" %u", static_cast<uint32_t>(item));
	std::printf("\n");
}

struct BucketMoveResult {
	cnt::darr<ObserverTrace> trace;
	bool hasTrueBucket = false;
	bool hasFalseBucket = false;
	uint32_t deleteHits = 0;
	uint32_t addHits = 0;
};

BucketMoveResult run_bucket_move(bool nonFragmenting) {
	TestWorld twld;

	const auto relationTrue = wld.add();
	const auto relationFalse = wld.add();
	const auto row = wld.add();
	const auto trigger = ecs::Pair(relationTrue, row);
	const auto result = ecs::Pair(relationFalse, row);

	if (nonFragmenting) {
		wld.add(relationTrue, ecs::Exclusive);
		wld.add(relationTrue, ecs::DontFragment);
		wld.add(relationFalse, ecs::Exclusive);
		wld.add(relationFalse, ecs::DontFragment);
	}

	wld.add(row, trigger);

	BucketMoveResult observed;
	observed.trace.reserve(4);
	bool inTrigger = false;
	bool moveRequested = false;
	uint32_t triggerHits = 0;
	uint32_t deleteHits = 0;
	uint32_t addHits = 0;
	const auto triggerObserver = wld.observer()
			.event(ecs::ObserverEvent::OnAdd)
			.all<Position>()
			.on_each([&](ecs::Entity entity) {
				++triggerHits;
				if (moveRequested)
					return;
				moveRequested = true;
				observed.trace.push_back(ObserverTrace::TriggerEnter);
				inTrigger = true;
				wld.del(entity, trigger);
				wld.add(entity, result);
				inTrigger = false;
				observed.trace.push_back(ObserverTrace::TriggerExit);
			})
			.entity();
	const auto deleteObserver = wld.observer()
			.event(ecs::ObserverEvent::OnDel)
			.all(trigger)
			.on_each([&](ecs::Iter& it) {
				deleteHits += it.size();
				if (inTrigger)
					observed.trace.push_back(ObserverTrace::PairDelete);
			})
			.entity();
	const auto addObserver = wld.observer()
			.event(ecs::ObserverEvent::OnAdd)
			.all(result)
			.on_each([&](ecs::Iter& it) {
				addHits += it.size();
				if (inTrigger)
					observed.trace.push_back(ObserverTrace::PairAdd);
			})
			.entity();
	(void)triggerObserver;
	(void)deleteObserver;
	(void)addObserver;

	wld.add<Position>(row);

	observed.hasTrueBucket = wld.has(row, trigger);
	observed.hasFalseBucket = wld.has(row, result);
	observed.deleteHits = deleteHits;
	observed.addHits = addHits;
	return observed;
}

} // namespace

TEST_CASE("Observer - nested component callback fires after the outer callback") {
	TestWorld twld;

	const auto entity = wld.add();
	bool outerActive = false;
	bool nestedWhileOuterActive = false;
	uint32_t outerHits = 0;
	uint32_t nestedHits = 0;

	const auto outerObserver = wld.observer()
			.event(ecs::ObserverEvent::OnAdd)
			.all<Position>()
			.no<Acceleration>()
			.on_each([&](ecs::Entity observed) {
				++outerHits;
				outerActive = true;
				wld.add<Acceleration>(observed);
				outerActive = false;
			})
			.entity();
	const auto nestedObserver = wld.observer()
			.event(ecs::ObserverEvent::OnAdd)
			.all<Acceleration>()
			.on_each([&](ecs::Entity) {
				++nestedHits;
				nestedWhileOuterActive |= outerActive;
			})
			.entity();
	(void)outerObserver;
	(void)nestedObserver;

	wld.add<Position>(entity);

	std::printf(
		"nested-component: outer=%u nested=%u synchronous=%u\n", outerHits, nestedHits,
		nestedWhileOuterActive ? 1U : 0U);
	CHECK(outerHits == 3);
	CHECK(nestedHits == 1);
	CHECK_FALSE(nestedWhileOuterActive);
	CHECK(wld.has<Acceleration>(entity));
}

//! Characterization only: this deliberately exercises the observed recursion; it is not an endorsement
//! of using an observer to mutate the term that triggers that same observer.
TEST_CASE("Observer - self-trigger reenters after one guarded mutation") {
	TestWorld twld;

	const auto entity = wld.add();
	constexpr uint32_t invocationCap = 1;
	uint32_t depth = 0;
	uint32_t maxDepth = 0;
	uint32_t hits = 0;
	bool actionTaken = false;

	const auto observer = wld.observer()
			.event(ecs::ObserverEvent::OnAdd)
			.all<Position>()
			.on_each([&](ecs::Entity observed) {
				++hits;
				++depth;
				if (depth > maxDepth)
					maxDepth = depth;
				if (!actionTaken) {
					actionTaken = true;
					wld.del<Position>(observed);
					wld.add<Position>(observed);
				}
				--depth;
			})
			.entity();
	(void)observer;

	wld.add<Position>(entity);

	std::printf("self-trigger: invocations=%u max-depth=%u action-cap=%u\n", hits, maxDepth, invocationCap);
	CHECK(hits == 3);
	CHECK(maxDepth == 2);
	CHECK(actionTaken);
	CHECK(depth == 0);
	CHECK(wld.has<Position>(entity));
}

TEST_CASE("Observer - same-event callbacks follow registration order") {
	TestWorld twld;

	const auto entity = wld.add();
	cnt::darr<uint32_t> order;
	order.reserve(3);

	const auto observer1 = wld.observer()
			.event(ecs::ObserverEvent::OnAdd)
			.all<Position>()
			.on_each([&](ecs::Entity) { order.push_back(1); })
			.entity();
	const auto observer2 = wld.observer()
			.event(ecs::ObserverEvent::OnAdd)
			.all<Position>()
			.on_each([&](ecs::Entity) { order.push_back(2); })
			.entity();
	const auto observer3 = wld.observer()
			.event(ecs::ObserverEvent::OnAdd)
			.all<Position>()
			.on_each([&](ecs::Entity) { order.push_back(3); })
			.entity();
	(void)observer1;
	(void)observer2;
	(void)observer3;

	wld.add<Position>(entity);

	std::printf("ordering:");
	for (const auto item: order)
		std::printf(" %u", item);
	std::printf("\n");
	CHECK(order.size() == 3);
	if (order.size() == 3) {
		CHECK(order[0] == 1);
		CHECK(order[1] == 2);
		CHECK(order[2] == 3);
	}
}

TEST_CASE("Observer - pair OnAdd and OnDel observe pair mutations") {
	TestWorld twld;

	const auto relation = wld.add();
	const auto target = wld.add();
	const auto source = wld.add();
	const auto pair = ecs::Pair(relation, target);
	uint32_t addHits = 0;
	uint32_t delHits = 0;

	const auto addObserver = wld.observer()
			.event(ecs::ObserverEvent::OnAdd)
			.all(pair)
			.on_each([&](ecs::Iter& it) { addHits += it.size(); })
			.entity();
	const auto delObserver = wld.observer()
			.event(ecs::ObserverEvent::OnDel)
			.all(pair)
			.on_each([&](ecs::Iter& it) { delHits += it.size(); })
			.entity();
	(void)addObserver;
	(void)delObserver;

	wld.add(source, pair);
	wld.del(source, pair);

	std::printf("pair-observer: add=%u del=%u\n", addHits, delHits);
	CHECK(addHits == 1);
	CHECK(delHits == 1);
}

TEST_CASE("Observer - bucket move fires pair observers for fragmenting relation") {
	const auto observed = run_bucket_move(false);

	print_trace("bucket-move-fragmenting", observed.trace);
	CHECK(observed.trace.size() == 4);
	if (observed.trace.size() == 4) {
		CHECK(observed.trace[0] == ObserverTrace::TriggerEnter);
		CHECK(observed.trace[1] == ObserverTrace::PairDelete);
		CHECK(observed.trace[2] == ObserverTrace::PairAdd);
		CHECK(observed.trace[3] == ObserverTrace::TriggerExit);
	}
	CHECK(observed.deleteHits == 1);
	CHECK(observed.addHits == 1);
	CHECK_FALSE(observed.hasTrueBucket);
	CHECK(observed.hasFalseBucket);
}

TEST_CASE("Observer - bucket move fires pair observers for non-fragmenting relation") {
	const auto observed = run_bucket_move(true);

	print_trace("bucket-move-non-fragmenting", observed.trace);
	CHECK(observed.trace.size() == 4);
	if (observed.trace.size() == 4) {
		CHECK(observed.trace[0] == ObserverTrace::TriggerEnter);
		CHECK(observed.trace[1] == ObserverTrace::PairDelete);
		CHECK(observed.trace[2] == ObserverTrace::PairAdd);
		CHECK(observed.trace[3] == ObserverTrace::TriggerExit);
	}
	CHECK(observed.deleteHits == 1);
	CHECK(observed.addHits == 1);
	CHECK_FALSE(observed.hasTrueBucket);
	CHECK(observed.hasFalseBucket);
}

#endif
