# Async timer cancellation

`Async` processes due deadline buckets in timer-ID order. A callback can cancel
itself, another timer, or all timers, and can create timers. Returning `true`
repeats the current delay; returning an integer sets the next delay. Other
supported results finish the timer.

Previously, repeats were held in a temporary batch while their live IDs were
removed. A later callback at the same deadline could not find a repeat to stop
it, and batch completion reinserted the repeat. The Windows regression timeout
in PR #59 ended with the repeating callback in `tests/async.gls.js` still
printing values; the exact same-deadline cancellation defect is reproducible
without depending on that script's wall-clock timing.

Dispatch now snapshots only the original bucket's IDs. Each ID is looked up
before invocation, so a canceled sibling does not run. Each surviving repeat
is immediately rescheduled before the next callback, keeping it visible to
cancellation. Self-cancellation and `StopTimers()` override a repeat result.
Timers created during dispatch remain scheduled for a subsequent bucket pass,
including those created at the original deadline.

The timer record is copied before invoking user code. Lookup, invocation,
removal, and repeat publication occur inside the existing GC accumulation, so
a callable remains alive while processing a result even if it canceled its
last timer reference. No map iterator is used after invoking a callback.

Exceptions still propagate. A throwing timer remains scheduled unless its
callback canceled it; unrun siblings remain scheduled, and earlier completed
callbacks remain completed. This change does not swallow errors, retry failed
callbacks automatically, add cross-thread timer mutation support, or change
the accepted delay range.

## Regression checks

The native `test async ...` cases set exact deadlines and dispatch one real
`Async::ProcessTimers` bucket through a narrow friend fixture. They do not
sleep, replace scheduling logic, or rely on clock resolution. Cleanup clears
pending timers even when an assertion fails, preventing a broken cancellation
from hanging the regression itself.

Coverage includes boolean/integer repeats canceled later in the same bucket,
cancellation of an unrun sibling, self-cancellation alongside another repeat,
`StopTimers()` followed by a nested timer, nested same-deadline insertion,
delay changes and one-shot completion, and exception propagation/bookkeeping.

On the unchanged scheduler, five of the initial seven cases failed immediately;
the ordinary repeat and stop-all controls passed. All eight final cases pass
after the fix. The full GSE suite, including the production async script,
passed locally under Clang 15 ASan/UBSan with `ASAN_OPTIONS=detect_leaks=1` and
`UBSAN_OPTIONS=halt_on_error=1` in 28.24 seconds. This is a local measurement,
not a hosted runtime guarantee. The existing local `-no-pie` sanitizer linker
workaround was retained; no repository toolchain flags were changed. Normal CTest
also passed all eight checks in 41.98 seconds; focused Clang-Tidy 15 analysis
of `Async.cpp` reported no findings.

Focused command (use the executable from the configured build):

```sh
./build/bin/GLSMAC --gse-tests --gse-tests-native-filter '^test async ' --quiet
```
