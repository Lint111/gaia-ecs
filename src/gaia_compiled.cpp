#define GAIA_COMPILED_MODE_IMPLEMENTATION 1
#include "gaia.h"

#if GAIA_COMPILED_MODE
namespace gaia::ecs {
	template void detail::CommandBuffer<AccessContextST, true>::commit();
	template void detail::CommandBuffer<AccessContextMT, true>::commit();
	template void detail::CommandBuffer<AccessContextST, false>::commit();
	template void detail::CommandBuffer<AccessContextMT, false>::commit();
} // namespace gaia::ecs
#endif
