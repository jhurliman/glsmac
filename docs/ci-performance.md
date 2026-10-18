# CI performance and reliability

## Portable movie probes

Ordinary gameplay changes continue to compile/link the production movie decoder
and run the complete selected regression suite. The additional standalone Linux
I/O cancellation and portable packaging probes run for their source/build/workflow
closure, manual/stable runs and explicit `ci:full` checkpoints. Their selection is
covered by `tools/tests/test_ci_scope.py`.

The FFmpeg source version and SHA256 remain pinned by
`cmake/PortableMovieFFmpeg.cmake`. CI caches only the installed codec libraries and
headers, source license, and matching source archive. Its exact cache key includes
OS, architecture, C compiler target/version, and the entire build recipe hash;
there is no fallback to an older recipe. A cache hit explicitly enables reuse;
missing artifacts and a mismatched source archive fail configuration. Probe C++
still compiles against the current checkout. Production compiler cache is saved
before probe execution so an unrelated probe failure cannot discard it.

Measurement at the pre-change reviewed movie head `a753fac1`, GitHub regression
run `39153550070`: Linux standalone probes consumed 37 seconds of the Linux
runner. This cost is eliminated for unrelated gameplay changes. Local WSL Linux,
Clang 15 probe compiler, pinned minimal FFmpeg 8.1.3: cold configure/build took
28.06 seconds, a fresh probe build with only the cached payload restored took
4.25 seconds (0.58 CPU seconds). A same-directory Ninja no-op took 0.06 seconds;
that is not used as the cache-restoration result. Both cold/restored probes passed
missing-file handling and the restored binary had no dynamic FFmpeg dependency.
The I/O cancellation probe passed separately. Incomplete cache and corrupt source
archive controls both failed configuration as intended. Hosted runner cache
transfer time and repeated CI stability remain to be measured on the reviewed PR.

A first attempt to cache ExternalProject stamps still re-extracted/reconfigured
in a fresh build directory (22.95 seconds). The final cache deliberately bypasses
those stamps through explicit validated reuse rather than reporting a no-op as a
warm cache test.
