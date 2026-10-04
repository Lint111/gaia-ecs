#pragma once
#include "gaia/config/config.h"

#include <atomic>
#include <cstdint>

namespace gaia {
	namespace ecs {
		//! Tests whether a change version is newer than a required version, including wraparound.
		//! \param changeVersion Version recorded when data last changed.
		//! \param requiredVersion Baseline version required by the caller.
		//! \return True when the change occurred after the baseline.
		GAIA_NODISCARD inline bool version_changed(uint32_t changeVersion, uint32_t requiredVersion) {
			// When a system runs for the first time, everything is considered changed.
			if GAIA_UNLIKELY (requiredVersion == 0U)
				return true;

			// Supporting wrap-around for version numbers. ChangeVersion must be
			// bigger than requiredVersion (never detect change of something the
			// system itself changed).
			return (int)(changeVersion - requiredVersion) > 0;
		}

		//! Reads a version counter that may be advanced by concurrent query workers.
		//! \param version Version counter to read.
		GAIA_NODISCARD inline uint32_t load_version(const uint32_t& version) {
			return std::atomic_ref<const uint32_t>(version).load(std::memory_order_acquire);
		}

		//! Advances a version counter while reserving zero as an invalid value.
		//! \param version Version counter to advance.
		inline void update_version(uint32_t& version) {
			auto atomicVersion = std::atomic_ref<uint32_t>(version);
			auto current = atomicVersion.load(std::memory_order_relaxed);
			uint32_t next = 0;
			do {
				next = current + 1U;
				// Handle wrap-around without publishing zero, which is reserved for systems
				// that have never run.
				if GAIA_UNLIKELY (next == 0U)
					next = 1U;
			} while (!atomicVersion.compare_exchange_weak(
					current, next, std::memory_order_acq_rel, std::memory_order_relaxed));
		}
	} // namespace ecs
} // namespace gaia
