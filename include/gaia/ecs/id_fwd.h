#pragma once
#include <cstdint>

namespace gaia {
	namespace ecs {
		//! Numeric portion shared by entity and component identifiers.
		using IdentifierId = uint32_t;
		//! Storage word used for packed identifier metadata.
		using IdentifierData = uint32_t;

		//! Numeric entity identifier type.
		using EntityId = IdentifierId;
		//! Numeric component identifier type.
		using ComponentId = IdentifierId;

		//! Cost class reported by relation degree queries.
		enum class CountKind : uint8_t;
	} // namespace ecs
} // namespace gaia
