# Gaia-ECS observer reentrancy characterization

This is a characterization of the current Gaia-ECS behavior. It does not change engine behavior and does not endorse mutating observer-triggering terms from callbacks.

## Tests

The probes are in [test_observer_reentrancy.cpp](../src/test/src/test_observer_reentrancy.cpp) and are registered in [src/test/CMakeLists.txt](../src/test/CMakeLists.txt:13).

The printed trace values are defined as:

| Value | Meaning |
| --- | --- |
| 1 | bucket-move trigger callback entered |
| 2 | `OnDel` for the old pair |
| 3 | `OnAdd` for the new pair |
| 4 | bucket-move trigger callback exited |

## Results

### 1. Nested firing

Test: `Observer - nested component callback fires after the outer callback` ([lines 105-145](../src/test/src/test_observer_reentrancy.cpp:105)). The `Position` observer adds `Acceleration`; a second observer watches `Acceleration`.

Observed output:

```text
nested-component: outer=3 nested=1 synchronous=0
```

The second observer fired once, but not while the first callback’s active flag was set. The outer `Position` observer was revisited three times during the structural transitions. This is observed, not guaranteed: the observer source dispatches callbacks through `execute_targets` ([observer_registry.inl:472-484](../include/gaia/ecs/observer_registry.inl:472)), but no source/API contract promises a deferred boundary, callback count, or reentrancy ordering.

### 2. Self-trigger

Test: `Observer - self-trigger reenters after one guarded mutation` ([lines 147-185](../src/test/src/test_observer_reentrancy.cpp:147)). The `OnAdd Position` callback removes and re-adds `Position` once; the callback contains an explicit one-action cap.

Observed output:

```text
self-trigger: invocations=3 max-depth=2 action-cap=1
```

The observer re-entered, reaching depth two, and the operation terminated only because the test stopped issuing further mutations. A separate exploratory version that continued issuing the same action until an invocation cap of eight was hit crashed with `SIGSEGV` before reaching that cap; therefore no safe unbounded-cycle behavior was inferred.

This is observed, not guaranteed. Gaia-ECS documents an active deletion stack for suppressing duplicate reentrant entity deletion ([world.h:660-662](../include/gaia/ecs/world.h:660)), but that is not a general component-observer recursion guard.

### 3. Ordering of three observers

Test: `Observer - same-event callbacks follow registration order` ([lines 187-225](../src/test/src/test_observer_reentrancy.cpp:187)). Three `OnAdd Position` observers append `1`, `2`, and `3`.

Observed output:

```text
ordering: 1 2 3
```

The current implementation appends observers to a Gaia-ECS `cnt::darr` in `add_observer_to_map` ([observer_registry.h:231-239](../include/gaia/ecs/observer_registry.h:231)) and iterates the relevant observer list in dispatch ([observer_registry.inl:472-485](../include/gaia/ecs/observer_registry.inl:472)). The result is registration order today, but it is observed, not guaranteed: those internal mechanics do not document registration order as API behavior.

### 4. Pair observers

Test: `Observer - pair OnAdd and OnDel observe pair mutations` ([lines 227-256](../src/test/src/test_observer_reentrancy.cpp:227)). It registers exact-pair `OnAdd` and `OnDel` observers, adds the pair, then removes it.

Observed output:

```text
pair-observer: add=1 del=1
```

Pair observers are supported. The source explicitly says observers may register pair terms before the exact pair exists ([observer_registry.inl:1130-1136](../include/gaia/ecs/observer_registry.inl:1130)), and registers pair terms in the normal `OnAdd`/`OnDel` maps ([observer_registry.inl:1143-1152](../include/gaia/ecs/observer_registry.inl:1143)). This is source-backed behavior, unlike the callback ordering/reentrancy timing above.

### 5. Bucket-move shape

Tests: `Observer - bucket move fires pair observers for fragmenting relation` and `Observer - bucket move fires pair observers for non-fragmenting relation` ([lines 258-290](../src/test/src/test_observer_reentrancy.cpp:258)). The row starts with `Pair(relationTrue, row)`. The trigger observer removes that pair and adds `Pair(relationFalse, row)` ([lines 56-70](../src/test/src/test_observer_reentrancy.cpp:56)). A guard prevents the trigger observer from issuing the move repeatedly when the pair transitions revisit its query.

Observed output for both storage modes:

```text
bucket-move-fragmenting: 1 2 3 4
bucket-move-non-fragmenting: 1 2 3 4
```

Thus, for the single guarded move, pair `OnDel` and pair `OnAdd` fired synchronously inside the trigger callback, in remove-then-add order, and the row ended in the false bucket only. This is observed, not guaranteed as a general reentrancy contract.

The unguarded exploratory callback repeatedly performed the same direct remove/add action and the test process crashed with `SIGSEGV`. That finding means a consumer must not assume an observer can repeat this mutation indefinitely; the passing characterization pins the terminating, one-move shape.

### 6. Non-fragmenting relations

The non-fragmenting test marks both custom relations `Exclusive` and `DontFragment` ([lines 40-44](../src/test/src/test_observer_reentrancy.cpp:40)). Gaia-ECS documents that only exclusive `DontFragment` relations use non-fragmenting relation storage ([world.h:889-900](../include/gaia/ecs/world.h:889)), and classifies that pair of flags as `NonFragmentingExclusive` ([world.h:2321-2334](../include/gaia/ecs/world.h:2321)).

Observed result: the non-fragmenting trace is identical to the fragmenting trace (`1 2 3 4`), with one delete, one add, and the expected final bucket membership. The source guarantees the storage classification, but not that observer reentrancy timing/order remains identical; that equivalence is observed, not guaranteed.

## Verification evidence

All heavy commands used the machine-wide queue because this checkout has no local `tools/with-test-lock.sh`; the authorized queue entry point was `/home/liory/Github/box-queue/queue/with-test-lock.sh`.

Release configure command:

```text
bash /home/liory/Github/box-queue/queue/with-test-lock.sh --agent w9-obsreentry --resource build --label obsreentry-baseline-config -- cmake -S . -B build-obsreentry -DGAIA_BUILD_UNITTEST=ON -DCMAKE_BUILD_TYPE=Release
```

Result: `UT_JOB_DONE build obsreentry-baseline-config exit=0 state=OK`.

Final Release build command:

```text
bash /home/liory/Github/box-queue/queue/with-test-lock.sh --agent w9-obsreentry --resource build --label obsreentry-final-build -- cmake --build build-obsreentry --target gaia_test -j 4
```

Verbatim final result lines:

```text
[  5%] Building CXX object src/test/CMakeFiles/gaia_test.dir/src/test_observer_reentrancy.cpp.o
[ 10%] Linking CXX executable gaia_test
[100%] Built target gaia_test
UT_JOB_DONE build obsreentry-final-build exit=0 state=OK
```

Focused characterization command:

```text
bash /home/liory/Github/box-queue/queue/with-test-lock.sh --agent w9-obsreentry --resource test --label obsreentry-final-focused-test -- ./build-obsreentry/src/test/gaia_test '--test-case=Observer - nested component callback fires after the outer callback,Observer - self-trigger reenters after one guarded mutation,Observer - same-event callbacks follow registration order,Observer - pair OnAdd and OnDel observe pair mutations,Observer - bucket move fires pair observers for fragmenting relation,Observer - bucket move fires pair observers for non-fragmenting relation'
```

Verbatim test output:

```text
[doctest] doctest version is "2.4.12"
[doctest] run with "--help" for options
nested-component: outer=3 nested=1 synchronous=0
self-trigger: invocations=3 max-depth=2 action-cap=1
ordering: 1 2 3
pair-observer: add=1 del=1
bucket-move-fragmenting: 1 2 3 4
bucket-move-non-fragmenting: 1 2 3 4
===============================================================================
[doctest] test cases:  6 |  6 passed | 0 failed | 635 skipped
[doctest] assertions: 33 | 33 passed | 0 failed |
[doctest] Status: SUCCESS!
UT_JOB_DONE test obsreentry-final-focused-test exit=0 state=OK
```

Final full-suite command:

```text
bash /home/liory/Github/box-queue/queue/with-test-lock.sh --agent w9-obsreentry --resource test --label obsreentry-final-full-test2 -- ./build-obsreentry/src/test/gaia_test
```

Verbatim final summary/output after the existing four deletion-policy error logs:

```text
nested-component: outer=3 nested=1 synchronous=0
self-trigger: invocations=3 max-depth=2 action-cap=1
ordering: 1 2 3
pair-observer: add=1 del=1
bucket-move-fragmenting: 1 2 3 4
bucket-move-non-fragmenting: 1 2 3 4
===============================================================================
[doctest] test cases:     641 |     641 passed | 0 failed | 0 skipped
[doctest] assertions: 2234270 | 2234270 passed | 0 failed |
[doctest] Status: SUCCESS!
UT_JOB_DONE test obsreentry-final-full-test2 exit=0 state=OK
```

The four deletion-policy error lines were present in the baseline full run as well; they are pre-existing test logging, not failures. The baseline was 635/635 test cases and 2,234,237/2,234,237 assertions.
