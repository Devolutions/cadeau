# Experiment: XMF VPX realtime defaults

## Background
Devolutions Gateway `video-streamer` hits a worst-case workload when it cuts into an uncued WebM stream and then re-encodes many consecutive frames as keyframes (to make the cut immediately decodable).

We observed a large performance regression (seconds per frame) with `xmf-master.dll`.
We also proved that a knobbed XMF implementation becomes fast when we force realtime-oriented libvpx settings (especially `g_lag_in_frames=0`).

## Goal
Validate that **changing XMF defaults** (without relying on env knobs) restores realtime performance (ms-level) in the same repro/bench.

## What this worktree changes
- `libxmf/XmfVpxEncoder.c`:
  - Set default `cfg.g_lag_in_frames = 0` (disable lookahead).
  - Set default `VP8E_SET_CPUUSED = 12` after init.
  - For VP9, enable row-mt and set tile columns/rows defaults (after init).

## How to build
Use VS dev environment + CMake Ninja build (Release).

Example:
```cmd
"C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 &&
cmake -G Ninja -S D:\cadeau-worktrees\xmf-vpx-realtime-defaults -B D:\cadeau-worktrees\.build-xmf-realtime-defaults -DCMAKE_BUILD_TYPE=Release -DWITH_XMF=ON -DUSE_CONAN=ON -DWITH_HALIDE=OFF -DCADEAU_INSTALL=OFF &&
cmake --build D:\cadeau-worktrees\.build-xmf-realtime-defaults --config Release
```

Then copy:
`D:\cadeau-worktrees\.build-xmf-realtime-defaults\lib\xmf.dll` → `D:\library\cadeau\xmf-realtime-defaults.dll`

## How to validate (Gateway repo)
Repro tests (video-streamer only):
- Set `DGATEWAY_LIB_XMF_PATH=D:\library\cadeau\xmf-realtime-defaults.dll`
- Run:
  - `cargo test -p video-streamer vpx_force_kf_spike_min_repro_attach_20s -- --ignored --nocapture`
  - `cargo test -p video-streamer vpx_force_only_first_cut_frame_min_repro_attach_20s -- --ignored --nocapture`

Bench:
- Set `DGATEWAY_LIB_XMF_PATH=D:\library\cadeau\xmf-realtime-defaults.dll`
- Run:
  - `cargo bench -p video-streamer --bench vpx_reencode`

