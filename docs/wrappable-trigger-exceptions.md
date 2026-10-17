# Event callback exception safety

`Wrappable::Trigger` copies the current handlers and releases the callback
mutex while running each handler, allowing reentrant handler changes. Its
guard now tracks that unlocked state with `std::unique_lock`. Any exception
from a handler propagates unchanged without unlocking the mutex a second time.
Normal handler order, the per-trigger handler snapshot, argument construction,
and expected-return-type checking remain unchanged.

Four native regressions exercise native C++ exceptions, nonstandard exceptions,
GSE exceptions, subsequent dispatch and registry changes, mutation of handlers
during dispatch, and recovery from a rejected return type. They run in the
existing GSE suite without assets or an extra default test invocation.

The original code hung on the second dispatch after the native exception.
GDB showed the main thread waiting for the callback mutex at
`Wrappable::Trigger` and the collector waiting for the active accumulation to
finish. The negative-control process was bounded to 40 seconds and terminated
after the stacks were captured.

Fixed validation: normal CTest 7/7 (30.38 s), full ASan/UBSan native/script
suite, and the existing correctness analysis for `Wrappable.cpp`. The sanitizer
run rebuilt both changed translation units with the exact parent commit's
sanitizer compilation flags and linked its unchanged objects; it did not modify
those objects. Leak detection was disabled for the separately recorded parser
exception leak baseline. No Windows runtime claim is made by these local runs.

Evidence is retained under
`/home/jhurliman/src/glsmac-wrappable-trigger-exceptions/`:
`negative-runtime.log`, `negative-gdb.log`, `negative-timeout.log`,
`fixed-ctest.log`, `asan-native.log`, and `analysis.log`.
