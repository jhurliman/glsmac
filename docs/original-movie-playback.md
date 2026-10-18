# Original movie playback

The initial intro now plays the installed `movies/logo.wve`, then
`movies/opening.wve` for `--ruleset smac` or `movies/openingx.wve` for SMAX.
With no ruleset selected it prefers the expansion intro when installed, falling
back to the original intro. Escape, Space, Enter or a mouse click skip directly
to the existing title menu. End of playback and unavailable/unsupported movies
also reach that menu. `--skipintro` still bypasses playback; returning from a game
does not replay the intro.

The reusable layer is `task::intro::Movie`, presented through the existing
OpenGL UI surface's `playmovie(path)`, `tickmovie()` and `stopmovie()` methods.
Surface removal/reset/shutdown cancels and joins decoding, closes the movie
audio device, and restores suspended engine sound. These methods do not attach
movies to research, captures, victories or any other gameplay event.

## Formats and timing

All 61 WVE files in the tested GOG Planetary Pack installation were probed.
They contain EA TQI/YUV420P video at 15 fps and EA ADPCM audio decoded to native
signed 16-bit stereo at 22,050 Hz. Video dimensions are 320×240 (22 files),
400×192 (37), 400×208 (1), and 592×288 (1). This includes faction, Secret Project,
intro and ending assets; their availability does not imply every gameplay event
has a movie.

The decoder accepts that measured codec/audio family. Other containers/codecs,
audio layouts/rates, changing dimensions, or video dimensions above 2048×2048
are rejected with a logged diagnostic. It never invokes the original external
player or transcodes/copies game assets. The installed logo has a malformed
trailing ADPCM block, independently reported by FFmpeg 4.4 and 8.1; bounded
packet recovery permits playback to finish and logs the damaged-packet count.
More than 32 damaged packets abort playback safely.

A worker owns FFmpeg and bounded queues (16 RGBA frames, two seconds of decoded
audio). The caller thread uploads due frames and queues at most about one second
of native PCM plus one decoded block to SDL. No decoding occurs in an audio
callback. The consumed SDL audio byte count drives video timing; `--nosound`
or audio-device failure uses a monotonic silent clock. Device failure is logged
separately from successful audio-device playback. SDL's output block is 1024
samples (about 46 ms); exact hardware latency and original-executable A/V parity
have not been measured. Late video frames are dropped to catch up to audio.

Movies retain their encoded aspect ratio inside a centered 4:3 viewport with a
black backdrop, including when the window is wider. CPU frame replacement also
increments the actor's texture revision so the existing UI cache displays
successive frames.

## Build dependencies

Movies default on and require FFmpeg development libraries `avformat`,
`avcodec`, `avutil`, and `swscale`. Linux uses the corresponding
`libavformat-dev libavcodec-dev libavutil-dev libswscale-dev` packages.
macOS uses Homebrew `ffmpeg`. Package discovery uses pkg-config, then
`FFMPEG_ROOT/include` and `FFMPEG_ROOT/lib`.

On Windows run `./tools/install_ffmpeg_windows.ps1`, then set `FFMPEG_ROOT`
to its printed directory before configuring the normal Windows preset. CI does
this automatically. The script pins a BtbN FFmpeg 8.1 LGPL shared development
archive and verifies its SHA256 before extraction; CMake stages the matching DLLs
and license next to the executable. Windows release packaging includes those
files. The pinned source version is `n8.1.3-18-g46d899d7a0`; build recipes and
release artifacts are at [BtbN/FFmpeg-Builds](https://github.com/BtbN/FFmpeg-Builds),
with upstream source at [FFmpeg](https://github.com/FFmpeg/FFmpeg).
No proprietary movie files are bundled.

`-DGLSMAC_MOVIES=OFF` explicitly builds without these libraries; intros then
log that playback is unavailable and continue into the title menu.

## Focused verification

- `GLSMAC --gse-tests --gse-tests-script GLSMAC_data/tests/intro_movies.gls.js --quiet`
  checks natural completion, repeated skip callbacks and missing assets.
- Native filter `^movie missing` checks missing-file completion and worker
  cancellation; the existing surface-ownership regression checks successive
  synthetic movie frame uploads and actor revision changes.
- `tools/tests/movie_playback_probe.cpp` is an optional installed-asset probe.
  Compile with `src/task/intro/Movie.cpp`, `-DGLSMAC_HAVE_MOVIES=1`, SDL2 and
  the four FFmpeg libraries. Pass a WVE path to play it, or add `cancel` to
  destroy players while their bounded decode queues are full.

Local checks used Linux FFmpeg 4.4 and Windows FFmpeg 8.1. The actual logo
decoded/presented 135 frames and 198,492 stereo sample frames in 9.002 seconds
on both; the Linux run also used a real system audio device without silent
fallback. Leak-enabled ASan/UBSan covered full logo playback, blocked-queue
cancellation and missing files. In-engine SMAX captures showed successive
scenes and Escape returned to the title menu; the original SMAC intro also
completed naturally into the menu. Native interactive macOS playback remains
unverified; its dependency/API compile path is covered by selected CI.
