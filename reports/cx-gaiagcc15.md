# cx-gaiagcc15

## LANDABLE NOW

GCC 15.2 now builds and passes the complete Gaia unit-test suite on the SoA pin in both required configurations.

## Upstream-alone result

Upstream main `8e9ebdcc` was tested in a separate detached worktree. The requested `gaia_test_query_access_assert` target failed before the fix with `-Werror=stringop-overflow` at `include/gaia/ecs/component_cache_item.h:749` and `include/gaia/ecs/observer.inl:85`. This confirms the failure is upstream and independent of the SoA pin commits.

The previous pin `84ba7f10` already contained and registered `gaia_test_query_access_assert`; it was not a new test. Its GCC 15 target build also failed at the same component-cache and observer diagnostics, with additional older-tree `threadpool.h:1270` and `component_cache_item.h:706` diagnostics.

Baseline evidence:

- upstream main target build: `/home/liory/codeman-cases/undertow/reports/cx-gaiagcc15-evidence/upstream-main-query-access-baseline.log`
- previous pin target build: `/home/liory/codeman-cases/undertow/reports/cx-gaiagcc15-evidence/previous-pin-query-access-baseline.log`
- pin pre-fix full Release build: `/home/liory/codeman-cases/undertow/reports/cx-gaiagcc15-evidence/pin-baseline-build.log`

## Fix

The component-name normalizer now checks the compile-time name length before copying and trims through a separately bounded index. Observer cached-index copying now has an explicit `queryIdCnt <= ChunkHeader::MAX_COMPONENTS` guard. The query sorter now passes `MAX_ITEMS_IN_QUERY` to the bounded sort overload, preventing GCC 15 from instantiating networks that can index beyond the fixed 12-element query arrays.

These changes preserve valid-input behavior and remove the compiler diagnostics at their source; no warning suppression, pragma, or test disabling was used.

## Commits

- Feature branch `pr/gcc15-stringop`: `d766a8fb` — `fix: make GCC 15 bounds provable in query paths` (based on upstream main `8e9ebdcc`).
- Pin branch `pin/soa31-upstream-2026-11-04`: `10630812` — cherry-picked fix on pin base `0720f891`.

## Verification

GCC: 15.2.0. All builds/tests used the live `/home/liory/.local/bin/with-test-lock.sh` queue with `UT_QUEUE_MEMORY_CAP_MAX_MB=56000` for builds.

| Tree | Configuration | Build | CTest |
|---|---|---:|---:|
| `pr/gcc15-stringop` `d766a8fb` | Release | pass | 53/53 passed |
| pin `10630812` | Release | pass | 53/53 passed |
| pin `10630812` | Debug | pass | 53/53 passed |

Fresh pin evidence:

- Release build: `/home/liory/codeman-cases/undertow/reports/cx-gaiagcc15-evidence/pin-release-build.log`
- Release CTest: `/home/liory/codeman-cases/undertow/reports/cx-gaiagcc15-evidence/pin-release-ctest.log`
- Debug build: `/home/liory/codeman-cases/undertow/reports/cx-gaiagcc15-evidence/pin-debug-build.log`
- Debug CTest: `/home/liory/codeman-cases/undertow/reports/cx-gaiagcc15-evidence/pin-debug-ctest.log`
- Feature full Release CTest: `/home/liory/codeman-cases/undertow/reports/cx-gaiagcc15-evidence/feature-full-release-ctest.log`

`single_include/gaia.h` was regenerated with `./make_single_header.sh` before both commits.

## Recovery ladder

- A: verified the live queue path and used it for every build/test/generation witness.
- B: regenerated `single_include/gaia.h` from source with `./make_single_header.sh` on both trees; no hand-edited generated content was used.
- C: the upstream-main target failure and the independent full-build sorter diagnostic were reproduced before semantic edits; targeted and full post-fix witnesses then passed.

## SPT DISPOSITION

- None; no SPT task id was supplied or worked.

## CONSOLIDATION ISSUES

- None.

