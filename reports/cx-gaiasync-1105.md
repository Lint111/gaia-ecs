# cx-gaiasync-1105

## LANDABLE NOW

Gaia fork branch `pin/soa31-upstream-2026-11-05` contains the complete rebase onto upstream `74d889eb0784d112ce4948bc6f7205db40194f00`. All seven commits from `origin/pin/soa31-upstream-2026-11-04` were retained; none has the same effect in upstream main.

The Gaia-only slice is verified and landable at the pre-report tip `cde38e70c3c11f2cb4f68556b296407633bcabb4`:

- SoA reflection now supports 31 members (`include/gaia/meta/reflection.h:74-76`), and SoA field sizes use 16-bit storage with a 65535-byte member limit (`include/gaia/ecs/component_desc.h:309-310,400-402`; `include/gaia/ecs/component_cache_item.h:141,847-849`).
- Component id packing preserves the 64-bit identifier while widening component size and encoding power-of-two alignment (`include/gaia/ecs/id.h:49-71`).
- GCC 15 bounds are explicit for type-name normalization, observer index copying, and fixed-capacity query sorting (`include/gaia/ecs/component_cache_item.h:722-727`; `include/gaia/ecs/observer.inl:84-87`; `include/gaia/ecs/query_common.h:2019-2021`).
- Boundary coverage includes 16- and 31-member SoA round trips (`src/test/src/test_storage.cpp:3112-3117`).
- `single_include/gaia.h` was regenerated with `./make_single_header.sh`; the build regenerated the same 92,442-line output.

Cherry-picked commit mapping:

| old | new | disposition |
|---|---|---|
| `f7608c5e` | `917b0255` | kept: 31-member SoA reflection and tests |
| `8127e1d5` | `0c3bdab9` | kept: 65535-byte SoA fields and bit layout |
| `211f7c27` | `0203518c` | kept: storage boundary test |
| `ddee5d92` | `c756f3ff` | kept: regenerated header and prior proposal inbox |
| `0720f891` | `5ff385a9` | kept: prior sync report/proposal record |
| `10630812` | `050eae73` | kept: GCC 15 query-path bounds fix |
| `e7b34c3a` | `cde38e70` | kept: prior GCC witness reports |

## STOPs

`STOPPED: federation native boot cannot validate the rebased revision until the consumer Gaia/VIXEN pin closure is bumped.` The boot-only consumer worktrees were not edited.

The override reached the Gaia worktree, but default configure stopped before composition or fixture tests at resolved VIXEN `dependencies/native/CMakeLists.txt:60-63`: it declared `a2e62491b56a8589c31da97128f0658088050a70` and fetched Gaia `cde38e70c3c11f2cb4f68556b296407633bcabb4`. The run summary reports unknown digests and no frames: `/home/liory/projects/undertow/.claude/worktrees/gaiasync-1105/.tmp/launcher-native-boot/runs/20261105T045955-2431477/summary.txt`; full log: `/home/liory/codeman-cases/undertow/reports/cx-gaiasync-1105-evidence/native-boot.log`.

The accepted current-pin comparison remains world `10fc17f580f12ea7` and save `1a643bc47268db02` (re-read from `undertow/reports/cx-savebench.md:92`). No new digest is claimed because the required consumer gate stopped at configure.

## PIN-BUMP PROPOSAL

Run a consumer pin-bump lane before the next federation witness:

1. Update Undertow `toolchain/providers/gaia.pin:9-10` from `84ba7f10dfabd376a862c6fc54d3d43ed365ae54` / `pin/soa31-2026-10-20` to this branch tip and keep the byte-identical kernel/VIXEN provider pin copies in sync.
2. Update the VIXEN native contract consumed by KFR at `dependencies/native/CMakeLists.txt:20,32-35` to the fork revision and repository, then rerun its exact-revision check at `:60-63`.
3. Advance Launcher `toolchain/providers/kfr.pin:1-3` to the KFR revision whose nested VIXEN contract carries the new Gaia revision. Launcher has no independent Gaia pin in this worktree.
4. Rerun `check-native-boot.sh` with default, composition, and fixtures; accept only matching world/save digests `10fc17f580f12ea7` / `1a643bc47268db02`.

## WITNESS

- Base: upstream main `74d889eb`; the old patch branch was 7 commits ahead and 3 commits behind it.
- Upstream-main baseline Release build, queued with the default cap, stopped at `36227MB > 32170MB` before completion (exit 143). Retrying the same full-width build with the supported `UT_QUEUE_MEMORY_CAP_MAX_MB=56000` exposed the intended pre-existing GCC 15 errors at `include/gaia/ecs/component_cache_item.h:749`, `include/gaia/ecs/observer.inl:85`, and `include/gaia/ecs/query_common.h` fixed-capacity sorting. No upstream test process started, so upstream test failure/hang rate is unavailable rather than zero.
- Rebased Release `-O2`: queued build `cx-gaiasync-1105:rebased-release-o2-build` passed; queued CTest passed `53/53`, `0` failed.
- Rebased Debug (`-g -Og`): queued build `cx-gaiasync-1105:rebased-debug-build` passed; queued CTest passed `53/53`, `0` failed.
- An additional default Release build/test at CMake `-O3` also passed `53/53`.
- The full suite covers the new SoA boundary tests and the GCC-sensitive query/observer paths; no targeted patch test failed or hung on the rebased branch.
- All build/test commands used `/home/liory/.local/bin/with-test-lock.sh`. The native gate was invoked directly because it queues its own configure/build/test phases, with `UNDERSET_BOOT_LOCK_TIMEOUT_SECONDS=7200` and `FETCHCONTENT_SOURCE_DIR_GAIA` set to this Gaia worktree.

## UPSTREAM CHANGES SINCE OLD BASE

- `9844f362`: geometric growth for several string, query, sparse/page-table, stamp, and entity-visit arrays removes repeated exact-size reallocations and improves growth-path performance.
- `dba13168`: world JSON now serializes plain entity tags without dereferencing a missing component-cache item, changing tag-save behavior from a crash to a named/unnamed tag value.
- `74d889eb`: the multithreading completion-order test waits until a worker reaches its deliberate stall, removing a load-sensitive test race; no Gaia runtime API change.

## SPT DISPOSITION

- No SPT task id was supplied; `show cx-gaiasync-1105` returned no item.

## CONSOLIDATION ISSUES (non-facade deltas this lane needed)

- proposed: Queued native configure failures lose the first CMake source location (SPT `0cf21591218c`; `.spt-proposals/cx-gaiasync-1105.jsonl`).
- existing proposal reused, not re-proposed: Gaia full CMake build exceeds the queue memory cap at default parallelism (SPT `1ce9509fad484cdef91702eea4ef5ce7ed140a7205e3994046917658e8173786`). This run reproduced `36227MB > 32170MB`; the full-width witness recovered with `UT_QUEUE_MEMORY_CAP_MAX_MB=56000` without reducing parallelism.
