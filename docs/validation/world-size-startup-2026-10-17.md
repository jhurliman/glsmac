# World size and startup-input checkpoint — 2026-10-17

The integrated source is `060b4ca1405664e18d64655dfbcfd1baeaeebf22`, retained as
`codex/validation-20261017-world-size-060b4ca1`. It adds the explicit accepted
SMAC world-size category and frozen five-row source profile, GlobalSettings V5
and world header V6, the positional RESOURCEINFO correction, pure faction
startup metadata, and inactive owned roster codecs to the preceding checkpoint.
The smaller publication PRs have separate trees and review/check requirements.

| Environment | Configuration | Result |
| --- | --- | --- |
| aleph0 native Windows | Clang Debug, full CTest | 8/8, 46.78 seconds |
| aleph0 Ubuntu/WSL | ASan/UBSan/LSan, leak detection and halt-on-error | 8/8, 66.82 seconds |
| honey native macOS arm64 | Apple Clang Debug, full CTest | 8/8, 36.18 seconds |

Linux retains the documented local non-PIE sanitizer workaround. These tests
exercise the actual setup scripts, events and settings/download boundaries
without a renderer. They do not establish rendered menu parity, complete
gameplay, live canonical roster publication or application of faction bonuses.
The user's earlier interactive Mac checks at `5cd78418` remain separate;
new-game latency is still untimed.

## Mac run and source identity

The isolated checkout is
`/Users/jhurliman/.codex-validation/glsmac-20261017-060b4ca1/src`.
It was cloned from the previous validation checkout and fetched from a local
bundle, then checked out at the exact commit above. Configure/build/tests ran
from 07:49:08 to 07:50:36 UTC on October 17, on macOS 26.5 (25F71), native arm64,
with Ninja and four build workers. The checkout was clean afterward. No
interactive game or original asset session was launched.

- Binary SHA-256:
  `44a8e01c93f4bcf8d33b0f7ca51081cd0762f8094107ff0a44b4a61e85bec3e9`.
- Transfer bundle SHA-256:
  `f83e19b250f79f4fb810c309aec7e7e30398d5ff8d736419894a908a307ba5c4`.

## Retained evidence and limits

Windows logs are `out/world-size-windows-{build,ctest}.log` in the integration
checkout. CTest SHA-256:
`3d420aa266ed4f86793cdc7045f67ded54120890cba5f3f1cab47a0cab47dc42`.

Linux logs are
`/home/jhurliman/src/glsmac-gc-audit/world-size-asan-{build,ctest}.log`.
CTest SHA-256:
`997566a1c6f48188811779a41279193dd148532d6fe5bc930bbbf0fc22b6aa7f`.

Mac logs are retained in the isolated checkout's sibling `logs/` directory and
copied to `out/validation-honey-060b4ca1/`. CTest SHA-256:
`3bf74401b9f331efa232ed097281284f4e3ff817fea46d6d6bc15b589f6210ba`.

The complete Linux correctness scan checked all 310 production translation units
at this checkpoint with LLVM 15.0.7 and two workers: zero failures in 220.12
seconds. The Windows-only network implementation is explicitly outside that
Linux scan. The report is
`/home/jhurliman/src/glsmac-gc-audit/world-size-analysis/summary.json`, SHA-256
`f26e494f7fe4aef8011e953ca07686f597ed43fbe44024a4822956078c6c82d4`.

The source-specific [world-size receipt](https://github.com/jhurliman/glsmac/blob/060b4ca1405664e18d64655dfbcfd1baeaeebf22/docs/smac-world-size-settings.md)
records installed-row comparisons, malformed-input and copied-production
negative controls, focused analysis and separate sanitizer runs. The source
contract explicitly rejects unsupported odd map heights rather than silently
rounding them, and preserves custom category 99 even at preset geometry.

This integration already contains the timer cancellation repair. Two older
publication branches that lacked it subsequently timed out in the existing
async repeat test: #120 on hosted macOS and #100 at `6201a114` on local Linux.
Their failure logs are retained; later checks incorporate the separate #69
prerequisite. The refreshed #69 publication passed 5/5 CTests in 26.34 seconds,
and #100 with that prerequisite passed 5/5 in 24.55 seconds. Those results do
not explain the separately retained Windows native-filter child timeout.
