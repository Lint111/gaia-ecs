# cx-gaiasync-1103 — Gaia SoA fork sync

## LANDABLE NOW

LANDABLE NOW on `pin/soa31-upstream-2026-11-03`.

- Upstream base: `49ad845884914e407c5900c4eb47a0d83538a38e` (`upstream/main`).
- Implementation tip before this report: `a16cb28c9c7dc099a7e2c41a37be56ce4f5b8f1a`.
- All three fork commits were kept; no equivalent effect exists in `upstream/main`:
  - `8a1bf89f` → `99ee25ae`, SoA reflection member limit 15 → 31.
  - `ccb1db01` → `f66886d3`, SoA member size 255 B → 65535 B.
  - `fa749e5f` → `102ad2ab`, closes the merged SoA storage test boundary.
- `a16cb28c` regenerates the committed `single_include/gaia.h` with `./make_single_header.sh`. The repository has no `clang-format`, so the generator explicitly skipped formatting.

The declaration and storage changes are visible at `include/gaia/meta/reflection.h:74`, `include/gaia/ecs/id.h:49-71`, `include/gaia/ecs/component_desc.h:309,391`, and `include/gaia/mem/data_layout_policy.h:358-380`. The user-facing limit is documented at `README.md:631`; regression coverage is at `src/test/src/test_storage.cpp:3112-3117` and `src/test/src/test_storage.cpp:3260-3277`.

## WITNESS

Required scope was the Gaia headers, the generated single header, and the complete `GAIA_BUILD_UNITTEST` CTest matrix.

- Release configure/build baseline on a detached clean `upstream/main` tree reproduced a pre-existing GCC 15 red before tests. The strict build fails with `-Werror=stringop-overflow` at `include/gaia/ecs/component_cache_item.h:749` and `include/gaia/ecs/observer.inl:85`, and `-Werror=array-bounds` at `include/gaia/cnt/impl/sarray_impl.h:162`. Baseline log: `/home/liory/.local/state/undertow/undertow-box-logs/1793691236-build-cx-gaiasync-1103:build-baseline-release.log`.
- Recovered Release configuration used the same `-O2` build with `-Wno-error=stringop-overflow -Wno-error=array-bounds`, and `cmake --build ... --parallel 1` to stay below the live queue memory cap. The rebased tree built all targets successfully.
- Release full CTest: 53/53 passed. SoA patch selection: 8/8 cases and 3814/3814 assertions passed.
- Debug full build: all targets built successfully with `--parallel 1`. Debug full CTest had 52/53 executable tests pass; the only failure was the unchanged timing-sensitive `src/test/src/test_mt.cpp:2460` (`FinalizeStall::claimed`), with 863/864 doctest cases passing. Rerunning only `gaia_test` passed 1/1 in 96.5 s; Debug SoA selection then passed 8/8 cases and 3814/3814 assertions. This is recorded as a transient branch-witness flake, not a patch failure.
- No runtime performance claim is made. No Gaia consumer or Undertow worktree was assigned to this lane, so the provider/FetchContent native boot was not run here. The latest paired native report records the expected reference digests as `world=10fc17f580f12ea7`, `save=1a643bc47268db02`; no consumer pin rejection was observed in this Gaia-only lane. A consumer pin-bump lane is not needed unless its independent witness rejects this commit.

## UPSTREAM CHANGES SINCE OLD BASE

- `13ef490a`: keeps empty archetypes for the documented lifespan; no conflict with the SoA metadata/storage changes.
- `c1f4a390`: corrects a README function name; the SoA README limit update remains additive.
- `49ad8458`: bounds JSON number parsing by the reader length; serialization behavior is current on the rebased tree and does not alter the SoA limit implementation.

## STOPs

None. The required Gaia deliverable is landable. The first Debug failure was recovered by a focused rerun; the strict upstream Release build remains a pre-existing GCC 15 warning-policy red outside this patch.

## CONTINUATION

None for the Gaia worktree. Federation/native boot and digest comparison should be performed by the consumer/launcher lane with this branch supplied through its documented Gaia provider override.

## CONSOLIDATION ISSUES

- proposed: Gaia Release builds on GCC 15 fail on unchanged -Werror diagnostics
- proposed: Gaia full CMake build exceeds the queue memory cap at default parallelism
- proposed: Gaia queued build wrapper can remain alive after gmake has emitted terminal errors
- proposed: Queued CTest failure diagnostics omit the failing test source location
