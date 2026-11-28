#pragma once
#include "gaia/config/config.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>

#include "gaia/cnt/fwd_llist.h"
#include "gaia/mt/spinlock.h"

namespace gaia {
	namespace mem {
		//! \cond INTERNAL
		namespace detail {
#if GAIA_ECS_TEST_HOOKS
			//! Allocator lifetime and drain work observed by the multithreading regressions.
			struct ArenaTestHooks {
				struct Live {
					int64_t arenas;
					int64_t bytes;
				};

				static Live live() noexcept {
					return {s_arenas.load(std::memory_order_relaxed), s_bytes.load(std::memory_order_relaxed)};
				}

				static void record_arena(int32_t count, int64_t bytes) noexcept {
					s_arenas.fetch_add(count, std::memory_order_relaxed);
					record_bytes(bytes);
				}

				static void record_bytes(int64_t bytes) noexcept {
					s_bytes.fetch_add(bytes, std::memory_order_relaxed);
				}

				static uint64_t remote_visits() noexcept {
					return s_remoteVisits;
				}

				static void visit_remote() noexcept {
					++s_remoteVisits;
				}

			private:
				inline static std::atomic_int64_t s_arenas{0};
				inline static std::atomic_int64_t s_bytes{0};
				inline static thread_local uint64_t s_remoteVisits = 0;
			};
#endif

			//! Synchronizes arena mutations and receives foreign frees while the owner is active.
			class ArenaState {
				template <typename>
				friend class ArenaRegistry;

				//! Aligned blocks leave this value available to mark a retired arena.
				static constexpr uintptr_t Closed = 1;
				std::atomic_uintptr_t m_remote{0};
#if GAIA_ALLOC_ARENA_LOCK
				inline static mt::SpinLock m_gate;
#else
				mt::SpinLock m_gate;
#endif

				//! Publishes a freed block. On success it may be reclaimed before this call returns.
				bool enqueue(void* block) noexcept {
					const auto address = reinterpret_cast<uintptr_t>(block);
					GAIA_ASSERT(address > Closed);
					auto head = m_remote.load(std::memory_order_relaxed);
					do {
						if (head == Closed)
							return false;

						std::memcpy(block, &head, sizeof(head));
					} while (
							!m_remote.compare_exchange_weak(head, address, std::memory_order_release, std::memory_order_relaxed));
					return true;
				}

				//! Detaches the current inbox under the arena gate.
				uintptr_t take_pending() noexcept {
					if (m_remote.load(std::memory_order_relaxed) <= Closed)
						return 0;
					return m_remote.exchange(0, std::memory_order_acquire);
				}

				//! Retires the owner and detaches all frees published before retirement.
				uintptr_t close() noexcept {
					return m_remote.exchange(Closed, std::memory_order_acquire);
				}
			};

			//! Provides a stateless allocator entry point, including for frees during TLS or static teardown.
			template <typename T>
			class arena_singleton final {
				arena_singleton() = delete;

			public:
				static T& get() noexcept {
					static T allocator;
					return allocator;
				}
			};

			//! Owns the arenas and coordinates explicit maintenance for one allocator category.
			template <typename Arena>
			class ArenaRegistry final {
				inline static mt::SpinLock s_registryGate;
				inline static cnt::fwd_llist<Arena> s_arenas;
				//! Only the owning thread reads this TLS slot. The live arena address is the owner identity.
				inline static thread_local Arena* s_current = nullptr;
				inline static thread_local bool s_ownerExited = false;

				struct ThreadOwner {
					ThreadOwner() {
						s_current = create();
					}

					~ThreadOwner() {
						auto* arena = s_current;
						s_current = nullptr;
						s_ownerExited = true;
						retire(*arena);
					}
				};

				static Arena* create() {
					auto* arena = new Arena();
					const std::lock_guard<mt::SpinLock> lock{s_registryGate};
					s_arenas.link(arena);
#if GAIA_ECS_TEST_HOOKS
					ArenaTestHooks::record_arena(1, int64_t(sizeof(Arena)));
#endif
					return arena;
				}

				//! Removes an empty retired arena after releasing its gate. Registry visitors may still hold a reference.
				static void destroy(Arena& arena) {
					{
						const std::lock_guard<mt::SpinLock> lock{s_registryGate};
						s_arenas.unlink(&arena);
#if GAIA_ECS_TEST_HOOKS
						ArenaTestHooks::record_arena(-1, -int64_t(sizeof(Arena)));
#endif
					}
					delete &arena;
				}

				static void drain(Arena& arena, uintptr_t head) {
					while (head != 0) {
						auto* block = reinterpret_cast<void*>(head);
						std::memcpy(&head, block, sizeof(head));
#if GAIA_ECS_TEST_HOOKS
						ArenaTestHooks::visit_remote();
#endif
						arena.free_local(block, false);
					}
				}

				static void retire(Arena& arena) {
					bool empty;
					{
						const std::lock_guard<mt::SpinLock> lock{arena.m_gate};
						drain(arena, arena.close());
						arena.trim(true);
						empty = arena.empty();
					}
					if (empty)
						destroy(arena);
				}

				static void* alloc_local(Arena& arena, uint32_t size) {
					const std::lock_guard<mt::SpinLock> lock{arena.m_gate};
					drain(arena, arena.take_pending());
					return arena.alloc_local(size);
				}

				static void* alloc_slow(uint32_t size) {
					if (s_ownerExited) {
						//! A late destructor can still allocate. Its block keeps the temporary arena alive.
						auto* arena = create();
						auto* block = alloc_local(*arena, size);
						retire(*arena);
						return block;
					}

					thread_local ThreadOwner owner;
					return alloc_local(*s_current, size);
				}

				static void free_retired(Arena& arena, void* block) {
					bool empty;
					{
						const std::lock_guard<mt::SpinLock> lock{arena.m_gate};
						arena.free_local(block, true);
						arena.verify();
						empty = arena.empty();
					}
					if (empty)
						destroy(arena);
				}

			public:
				static void* alloc(uint32_t size) {
					auto* arena = s_current;
					if GAIA_UNLIKELY (arena == nullptr)
						return alloc_slow(size);
					return alloc_local(*arena, size);
				}

				//! A live block pins its arena until published or freed under the gate.
				static void free(Arena& arena, void* block) {
					if (&arena == s_current) {
						const std::lock_guard<mt::SpinLock> lock{arena.m_gate};
						drain(arena, arena.take_pending());
						arena.free_local(block, false);
						arena.verify();
					} else if (!arena.enqueue(block)) {
						free_retired(arena, block);
					}
				}

				static void flush_local(bool releaseAll) {
					with_local([releaseAll](Arena& arena) {
						drain(arena, arena.take_pending());
						arena.trim(releaseAll);
					});
				}

				template <typename Func>
				static void with_local(Func fn) {
					if (auto* arena = s_current; arena != nullptr) {
						const std::lock_guard<mt::SpinLock> lock{arena->m_gate};
						fn(*arena);
					}
				}

				//! Visits each registered arena under its gate, draining frees published before its turn.
				template <typename Func>
				static void visit(Func fn) {
					const std::lock_guard<mt::SpinLock> registryLock{s_registryGate};
					for (auto& arena: s_arenas) {
						const std::lock_guard<mt::SpinLock> arenaLock{arena.m_gate};
						drain(arena, arena.take_pending());
						fn(arena);
					}
				}
			};
		} // namespace detail
		//! \endcond
	} // namespace mem
} // namespace gaia
