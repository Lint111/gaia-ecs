#pragma once
#include "gaia/config/config.h"

#include <cinttypes>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "gaia/cnt/fwd_llist.h"
#include "gaia/core/utility.h"
#include "gaia/mem/arena.h"
#include "gaia/mem/mem_alloc.h"
#include "gaia/meta/type_info.h"
#include "gaia/util/logging.h"

namespace gaia {
	namespace mem {
		//! Required alignment of each page block.
		static constexpr uint32_t MemoryBlockAlignment = 16;
		//! Default usable size in bytes of one memory block.
		//! 32 kiB per block by default.
		static constexpr uint32_t MemoryBlockBytesDefault = 32768;
		//! Header area preceding each aligned usable block.
		static constexpr uint32_t MemoryBlockUsableOffset = MemoryBlockAlignment;
		static_assert(MemoryBlockUsableOffset > sizeof(uintptr_t));

		//! Common header for allocator pages.
		struct GAIA_API MemoryPageHeader {
			//! Pointer to data managed by page
			void* m_data;
			//! Owner state for the allocation arena that created this page
			detail::ArenaState* m_arena;

			//! Creates a page header for a backing allocation.
			//! \param ptr Backing allocation address.
			//! \param arena Arena that owns the page.
			MemoryPageHeader(void* ptr, detail::ArenaState* arena): m_data(ptr), m_arena(arena) {}
		};

		//! Fixed-capacity page of equal-sized blocks.
		//! \tparam T Allocation category used for singleton separation and diagnostics.
		//! \tparam RequestedBlockSize Requested bytes per block, or zero for the default.
		template <typename T, uint32_t RequestedBlockSize>
		struct alignas(64) MemoryPage: MemoryPageHeader, cnt::fwd_llist_base<MemoryPage<T, RequestedBlockSize>> {
			//! Rounds a size up to MemoryBlockAlignment.
			//! \param num Size in bytes.
			//! \return Aligned size in bytes.
			static constexpr uint32_t next_multiple_of_alignment(uint32_t num) {
				return (num + (MemoryBlockAlignment - 1)) & uint32_t(-(int32_t)MemoryBlockAlignment);
			}
			//! Selects and aligns the configured block size, including its header.
			//! \return Total block size in bytes.
			static constexpr uint32_t calculate_block_size() {
				constexpr uint32_t UsableBytes = RequestedBlockSize == 0 ? MemoryBlockBytesDefault : RequestedBlockSize;
				return next_multiple_of_alignment(UsableBytes + MemoryBlockUsableOffset);
			}

			//! Size of one block in bytes.
			static constexpr uint32_t MemoryBlockBytes = calculate_block_size();
			static_assert(MemoryBlockBytes >= MemoryBlockUsableOffset + sizeof(uintptr_t));
			//! Maximum number of blocks in a page.
			static constexpr uint16_t NBlocks = 48;
			//! Sentinel terminating the recycled-block list.
			static constexpr uint32_t InvalidBlockId = NBlocks + 1;
#if GAIA_DEBUG
			static constexpr uint8_t FreedBlockPattern = 0xDD;
			static constexpr uintptr_t FreedPageMarker = ~(uintptr_t)0;
#endif

			//! Number of blocks in the block array
			uint8_t m_blockCnt;
			//! Number of used blocks out of NBlocks
			uint8_t m_usedBlocks;
			//! Index of the next block to recycle
			uint8_t m_nextFreeBlock;
			//! Number of blocks to recycle
			uint8_t m_freeBlocks;

			//! Creates an empty page over a backing allocation.
			//! \param ptr Backing allocation address.
			//! \param arena Arena that owns the page.
			MemoryPage(void* ptr, detail::ArenaState* arena):
					MemoryPageHeader(ptr, arena), m_blockCnt(0), m_usedBlocks(0), m_nextFreeBlock(0), m_freeBlocks(0) {
				static_assert(sizeof(MemoryPage) == 64);
			}

			//! Writes a recycle link in the block prefix after its page pointer.
			//! \param blockIdx Block whose link is updated.
			//! \param value Linked block index or InvalidBlockId.
			void write_block_idx(uint32_t blockIdx, uint32_t value) {
				GAIA_ASSERT(blockIdx < NBlocks);
				GAIA_ASSERT(value <= InvalidBlockId);
				auto* block = (uint8_t*)m_data + blockIdx * MemoryBlockBytes;
				block[sizeof(uintptr_t)] = (uint8_t)value;
			}

			//! Reads the recycle link in a free block.
			//! \param blockIdx Block whose link is read.
			//! \return Linked block index or InvalidBlockId.
			uint8_t read_block_idx(uint32_t blockIdx) const {
				GAIA_ASSERT(blockIdx < NBlocks);
				const auto* block = (const uint8_t*)m_data + blockIdx * MemoryBlockBytes;
				return block[sizeof(uintptr_t)];
			}

			//! Allocate a new block for this page.
			//! \return Pointer to the usable block storage.
			GAIA_NODISCARD void* alloc_block() {
				auto StoreBlockAddress = [&](uint32_t index) {
					// Encode info about the block's page in the memory block.
					// The actual pointer returned is offset by MemoryBlockUsableOffset bytes
					uint8_t* pMemoryBlock = (uint8_t*)m_data + (index * MemoryBlockBytes);
					GAIA_ASSERT((uintptr_t)pMemoryBlock % MemoryBlockAlignment == 0);
					mem::unaligned_ref<uintptr_t>{pMemoryBlock} = (uintptr_t)this;
					return (void*)(pMemoryBlock + MemoryBlockUsableOffset);
				};

				// We don't want to go out of range for new blocks
				GAIA_ASSERT(!full() && "Trying to allocate too many blocks!");

				if (m_freeBlocks == 0U) {
					const auto index = m_blockCnt;
					++m_usedBlocks;
					++m_blockCnt;

					return StoreBlockAddress(index);
				}

				GAIA_ASSERT(m_nextFreeBlock < m_blockCnt && "Block allocator recycle list broken!");

				++m_usedBlocks;
				--m_freeBlocks;

				const auto index = m_nextFreeBlock;
				m_nextFreeBlock = read_block_idx(m_nextFreeBlock);

				return StoreBlockAddress(index);
			}

			//! Release the block allocated by this page.
			//! \param pBlock Pointer previously returned by alloc_block().
			void free_block(void* pBlock) {
				GAIA_ASSERT(m_usedBlocks > 0);
				GAIA_ASSERT(m_freeBlocks <= NBlocks);

				auto ReadBlockAddress = [&](void* pMemory) {
					// Offset the chunk memory so we get the real block address
					const auto* pMemoryBlock = (uint8_t*)pMemory - MemoryBlockUsableOffset;
#if GAIA_ASSERT_ENABLED
					const auto pageAddr = (uintptr_t)mem::unaligned_ref<uintptr_t>{(void*)pMemoryBlock};
					GAIA_ASSERT(pageAddr == (uintptr_t)this);
#endif
					const auto blckAddr = (uintptr_t)pMemoryBlock;
					GAIA_ASSERT(blckAddr % 16 == 0);
					const auto dataAddr = (uintptr_t)m_data;
					const auto blockIdx = (uint32_t)((blckAddr - dataAddr) / MemoryBlockBytes);
					return blockIdx;
				};
				const auto blockIdx = ReadBlockAddress(pBlock);

#if GAIA_DEBUG
				mem::unaligned_ref<uintptr_t>{(uint8_t*)pBlock - MemoryBlockUsableOffset} = FreedPageMarker;
				std::memset(pBlock, FreedBlockPattern, MemoryBlockBytes - MemoryBlockUsableOffset);
#endif

				// Update our implicit list
				if (m_freeBlocks == 0U)
					write_block_idx(blockIdx, InvalidBlockId);
				else
					write_block_idx(blockIdx, m_nextFreeBlock);
				m_nextFreeBlock = (uint8_t)blockIdx;

				++m_freeBlocks;
				--m_usedBlocks;
			}

			//! Returns the number of live blocks.
			//! \return Number of allocated blocks.
			GAIA_NODISCARD uint32_t used_blocks_cnt() const {
				return m_usedBlocks;
			}

			//! Reports whether all page blocks are allocated.
			//! \return True when the page is full.
			GAIA_NODISCARD bool full() const {
				return used_blocks_cnt() >= NBlocks;
			}

			//! Reports whether no page blocks are allocated.
			//! \return True when the page is empty.
			GAIA_NODISCARD bool empty() const {
				return used_blocks_cnt() == 0;
			}

			//! Verifies page invariants in assertion-enabled builds.
			void verify() const {
#if GAIA_ASSERT_ENABLED
				GAIA_ASSERT(m_blockCnt <= NBlocks);
				GAIA_ASSERT(m_usedBlocks <= m_blockCnt);
				GAIA_ASSERT(m_freeBlocks <= m_blockCnt);
				GAIA_ASSERT(m_usedBlocks + m_freeBlocks == m_blockCnt);
				GAIA_ASSERT(((uintptr_t)m_data % MemoryBlockAlignment) == 0);

				uint64_t freeMask = 0;
				if (m_freeBlocks != 0) {
					uint32_t next = m_nextFreeBlock;
					GAIA_FOR(m_freeBlocks) {
						GAIA_ASSERT(next < m_blockCnt);
						const auto bit = uint64_t(1) << next;
						GAIA_ASSERT((freeMask & bit) == 0 && "Free list contains a cycle");
						freeMask |= bit;
						next = read_block_idx(next);
					}

					GAIA_ASSERT(next == InvalidBlockId);
				}

				GAIA_FOR(m_blockCnt) {
					const auto* pMemoryBlock = (const uint8_t*)m_data + (i * MemoryBlockBytes);
					GAIA_ASSERT(((uintptr_t)pMemoryBlock % MemoryBlockAlignment) == 0);

	#if GAIA_DEBUG
					const bool isFree = (freeMask & (uint64_t(1) << i)) != 0;
					const auto pageAddr = (uintptr_t)mem::unaligned_ref<uintptr_t>{(void*)pMemoryBlock};
					GAIA_ASSERT(pageAddr == (isFree ? FreedPageMarker : (uintptr_t)this));
	#endif
				}
#endif
			}
		};

		//! Lists the non-full and full pages belonging to an allocator.
		//! \tparam T Allocation category.
		//! \tparam RequestedBlockSize Requested bytes per block.
		template <typename T, uint32_t RequestedBlockSize>
		struct MemoryPageContainer {
			//! List of available pages
			cnt::fwd_llist<MemoryPage<T, RequestedBlockSize>> pagesFree;
			//! List of full pages
			cnt::fwd_llist<MemoryPage<T, RequestedBlockSize>> pagesFull;

			//! Reports whether the container has no pages.
			//! \return True when both page lists are empty.
			GAIA_NODISCARD bool empty() const {
				return pagesFree.first == nullptr && pagesFull.first == nullptr;
			}
		};

		//! Aggregate statistics for a paged allocator.
		struct GAIA_API MemoryPageStats final {
			//! Total allocated memory
			uint64_t mem_total;
			//! Memory actively used
			uint64_t mem_used;
			//! Number of allocated pages
			uint32_t num_pages;
			//! Number of free pages
			uint32_t num_pages_free;
		};

		//! \cond INTERNAL
		namespace detail {
			template <typename T, uint32_t RequestedBlockSize>
			class PagedAllocatorImpl;
		}
		//! \endcond

		//! Shared fixed-block allocator for an allocation category.
		//! \tparam T Allocation category used for singleton separation and diagnostics.
		//! \tparam RequestedBlockSize Requested bytes per block, or zero for the default.
		template <typename T, uint32_t RequestedBlockSize = 0>
		using PagedAllocator = detail::arena_singleton<detail::PagedAllocatorImpl<T, RequestedBlockSize>>;

		//! \cond INTERNAL
		namespace detail {

			template <typename T, uint32_t RequestedBlockSize>
			class PagedAllocatorArena final:
					public ArenaState,
					public cnt::fwd_llist_base<PagedAllocatorArena<T, RequestedBlockSize>> {
				using Page = MemoryPage<T, RequestedBlockSize>;
				MemoryPageContainer<T, RequestedBlockSize> m_pages;

				//! Diagnostic labels are shared by all arenas for this category.
				struct Labels {
					char pageData[256]{};
					char memPage[256]{};

					Labels() {
						const auto typeName = meta::type_info::name<T>();
						constexpr auto MaxLength = sizeof(pageData) - sizeof("PageData_");
						const auto length = typeName.size() < MaxLength ? typeName.size() : MaxLength;
						GAIA_STRCPY(pageData, sizeof(pageData), "PageData_");
						memcpy(pageData + 9, typeName.data(), length);
						GAIA_STRCPY(memPage, sizeof(memPage), "MemPage_");
						memcpy(memPage + 8, typeName.data(), length);
					}
				};

				static const Labels& labels() {
					static const Labels value;
					return value;
				}

			public:
				//! Allocates memory
				void* alloc_local(uint32_t) {
					void* pBlock = nullptr;

					// Find first page with available space
					auto* pPage = m_pages.pagesFree.first;
					GAIA_ASSERT(pPage == nullptr || !pPage->full());
					if (pPage == nullptr) {
						// Allocate a new page if no free page was found
						pPage = alloc_page();
						m_pages.pagesFree.link(pPage);
					}

					// Allocate a new chunk of memory
					pBlock = pPage->alloc_block();

					// Handle full pages
					if (pPage->full()) {
						// Remove the page from the open list
						m_pages.pagesFree.unlink(pPage);
						// Move our page to the full list
						m_pages.pagesFull.link(pPage);
					}

					verify();
					return pBlock;
				}

				GAIA_CLANG_WARNING_PUSH()
				// Memory is aligned so we can silence this warning
				GAIA_CLANG_WARNING_DISABLE("-Wcast-align")

				static Page* page_from_block(void* pBlock) {
					const auto address = (uintptr_t)mem::unaligned_ref<uintptr_t>{(uint8_t*)pBlock - MemoryBlockUsableOffset};
					GAIA_ASSERT(address % alignof(Page) == 0);
					return (Page*)address;
				}

				void free_local(void* pBlock, bool releaseEmpty) {
					auto* pPage = page_from_block(pBlock);
					const bool wasFull = pPage->full();

					GAIA_ASSERT(pPage->get_fwd_llist_link().linked());

					// Free the chunk
					pPage->free_block(pBlock);

					// Update lists
					if (wasFull) {
						// Our page is no longer full
						m_pages.pagesFull.unlink(pPage);
						// Move our page to the open list
						m_pages.pagesFree.link(pPage);
					}

					if (releaseEmpty && pPage->empty()) {
						m_pages.pagesFree.unlink(pPage);
						free_page(pPage);
					}
				}

				GAIA_CLANG_WARNING_POP()

				//! Returns allocator statistics
				MemoryPageStats stats() const {
					MemoryPageStats stats{};

					stats.num_pages = (uint32_t)m_pages.pagesFree.size() + (uint32_t)m_pages.pagesFull.size();
					stats.num_pages_free = (uint32_t)m_pages.pagesFree.size();
					stats.mem_total = stats.num_pages * (size_t)Page::MemoryBlockBytes * Page::NBlocks;
					stats.mem_used = m_pages.pagesFull.size() * (size_t)Page::MemoryBlockBytes * Page::NBlocks;
					for (const auto& page: m_pages.pagesFree)
						stats.mem_used += page.used_blocks_cnt() * (size_t)Page::MemoryBlockBytes;

					return stats;
				}

				//! Flushes unused memory
				void trim(bool) {
					for (auto it = m_pages.pagesFree.begin(); it != m_pages.pagesFree.end();) {
						auto* pPage = &(*it);
						++it;

						// Skip non-empty pages
						if (!pPage->empty())
							continue;

						m_pages.pagesFree.unlink(pPage);
						free_page(pPage);
					}

					verify();
				}

				void verify() const {
#if GAIA_ASSERT_ENABLED
					for (const auto& page: m_pages.pagesFree) {
						GAIA_ASSERT(page.get_fwd_llist_link().linked());
						GAIA_ASSERT(!page.full());
						page.verify();
					}

					for (const auto& page: m_pages.pagesFull) {
						GAIA_ASSERT(page.get_fwd_llist_link().linked());
						GAIA_ASSERT(page.full());
						page.verify();
					}
#endif
				}

				bool empty() const {
					return m_pages.empty();
				}

			private:
				Page* alloc_page() {
					const uint32_t size = Page::NBlocks * Page::MemoryBlockBytes;
					const auto& names = labels();
					auto* data = mem::AllocHelper::alloc_alig<uint8_t>(names.pageData, MemoryBlockAlignment, size);
					auto* page = mem::AllocHelper::alloc_alig<Page>(names.memPage, alignof(Page));
#if GAIA_ECS_TEST_HOOKS
					ArenaTestHooks::record_bytes(int64_t(size + sizeof(Page)));
#endif
					return new (page) Page(data, this);
				}

				static void free_page(Page* page) {
					GAIA_ASSERT(page != nullptr);
					const auto& names = labels();
#if GAIA_ECS_TEST_HOOKS
					ArenaTestHooks::record_bytes(-int64_t(Page::NBlocks * Page::MemoryBlockBytes + sizeof(Page)));
#endif
					mem::AllocHelper::free_alig(names.pageData, page->m_data);
					page->~MemoryPage();
					mem::AllocHelper::free_alig(names.memPage, page);
				}
			};

			template <typename T, uint32_t RequestedBlockSize>
			class PagedAllocatorImpl final {
				friend ::gaia::mem::PagedAllocator<T, RequestedBlockSize>;

				using Arena = PagedAllocatorArena<T, RequestedBlockSize>;
				using Arenas = ArenaRegistry<Arena>;

				PagedAllocatorImpl() = default;
				PagedAllocatorImpl(const PagedAllocatorImpl&) = delete;
				PagedAllocatorImpl& operator=(const PagedAllocatorImpl&) = delete;

			public:
				GAIA_NODISCARD void* alloc(uint32_t dummy) {
					return Arenas::alloc(dummy);
				}

				void free(void* pBlock) {
					GAIA_ASSERT(pBlock != nullptr);
					if (pBlock == nullptr)
						return;
					auto* page = Arena::page_from_block(pBlock);
					Arenas::free(static_cast<Arena&>(*page->m_arena), pBlock);
				}

				//! Drains pending frees and releases idle pages across all arenas of this specialization.
				void flush() {
					Arenas::visit([](Arena& arena) {
						arena.trim(true);
					});
				}

				//! Reports the calling thread's allocation statistics.
				GAIA_NODISCARD MemoryPageStats stats() const {
					MemoryPageStats stats{};
					Arenas::with_local([&](Arena& arena) {
						stats = arena.stats();
					});
					return stats;
				}

				//! Performs diagnostics of the memory used.
				void diag() const {
					MemoryPageStats memStats{};
					Arenas::visit([&](Arena& arena) {
						const auto current = arena.stats();
						memStats.mem_total += current.mem_total;
						memStats.mem_used += current.mem_used;
						memStats.num_pages += current.num_pages;
						memStats.num_pages_free += current.num_pages_free;
					});
					GAIA_LOG_N("PagedAllocator %p stats", (void*)this);
					GAIA_LOG_N("  Allocated: %" PRIu64 " B", memStats.mem_total);
					GAIA_LOG_N("  Used: %" PRIu64 " B", memStats.mem_used);
					GAIA_LOG_N("  Free capacity: %" PRIu64 " B", memStats.mem_total - memStats.mem_used);
					GAIA_LOG_N(
							"  Utilization: %.1f%%",
							memStats.mem_total != 0 ? 100.0 * ((double)memStats.mem_used / (double)memStats.mem_total) : 0.0);
					GAIA_LOG_N("  Pages: %u", memStats.num_pages);
					GAIA_LOG_N("  Free pages: %u", memStats.num_pages_free);
				}

				void verify() const {
#if GAIA_ASSERT_ENABLED
					Arenas::with_local([](Arena& arena) {
						arena.verify();
					});
#endif
				}
			};

		} // namespace detail
		//! \endcond
	} // namespace mem
} // namespace gaia
