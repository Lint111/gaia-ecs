# Fix GCC 15 `-Werror` bounds diagnostics in ECS query paths

## Summary

GCC 15.2 diagnoses three existing fixed-capacity assumptions in optimized Gaia unit-test builds:

- component-name normalization trims through a compiler-opaque `uint32_t` index;
- observer index caching copies `plan.termCount` entries into a 32-byte fixed array without a visible bound;
- query-term sorting uses the unbounded pointer overload even though the query capacity is 12.

The patch makes those bounds explicit without changing behavior for valid Gaia inputs. Oversized compile-time component names now return before copying, observer index copying is guarded by the destination capacity, and the sorter receives its existing `MAX_ITEMS_IN_QUERY` bound so larger sorting networks are not instantiated for a 12-element buffer.

## Reproduction

Environment: GCC 15.2.0, CMake 3.31.6, Ninja 1.12.1.

```bash
cmake -S . -B build-gcc15 -G Ninja -DCMAKE_BUILD_TYPE=Release -DGAIA_BUILD_UNITTEST=ON
cmake --build build-gcc15 --target gaia_test_query_access_assert -v
```

Before `d766a8fb`, upstream main `8e9ebdcc` failed with `-Werror=stringop-overflow` at `component_cache_item.h:749` and `observer.inl:85`. After the patch, the target build passed and all 42 `gaia_test_query_access_assert_*` scenarios passed.

The complete upstream-main Release build also exposed GCC 15 `-Werror=array-bounds` in the query sorter; the explicit `MAX_ITEMS_IN_QUERY` bound fixes that independently and the full build then passed.

## Validation

- Upstream feature branch `pr/gcc15-stringop` at `d766a8fb`: full Release build passed; CTest passed 53/53.
- Pinned SoA branch at `10630812`: Release build passed and CTest passed 53/53; Debug build passed and CTest passed 53/53.

