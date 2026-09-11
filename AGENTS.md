# AGENTS.md — mpz-chiyo-chan / mpz-janne

Forks of [mpz](https://github.com/olegantonyan/mpz) (Qt6/C++ folder music player).
Branches: `master` (upstream mirror), `ponytail-fixes` (chiyo-chan, default),
`janne-minimal` (minimal fork). Never push to `origin` (upstream) — remotes:
`origin` = upstream (fetch only), `fork` = chiyo-chan repo.

## Building (READ THIS OR YOU WILL OOM THE MACHINE)

- Host lacks Qt6Multimedia. Build ONLY inside the KDE SDK flatpak:
  `flatpak run --user --filesystem=home --command=bash org.kde.Sdk//6.11`
- Configure once per build dir: `cmake -B build-sdk -DCMAKE_BUILD_TYPE=Release
  -DENABLE_UPDATE_CHECK=OFF -DENABLE_QHOTKEY=OFF`
- Build with `nice -n 19 cmake --build build-sdk --parallel 1` (single job!).
  `-j12` once ate 12.8G RAM + 9.8G swap and `earlyoom` killed the whole
  Cinnamon session. Never exceed `-j1`. Run builds in background (nohup),
  never poll in a loop — one tiny check at a time.
- Two build dirs: `build-sdk/` (chiyo, branch ponytail-fixes),
  `build-sdk-janne/` (minimal, branch janne-minimal). Always `git checkout`
  the right branch before building that dir.
- Install: `~/.local/bin/mpz` (chiyo), `~/.local/bin/mpz-janne` (minimal).
  Binaries need the SDK to run; launch via the Desktop icons
  (`mpz-chiyo.desktop`, `mpz-janne.desktop`) which wrap the flatpak call.
  If `cp` says "Text file busy", the player is running — stage to `mpz.new`.

## Gotchas learned the hard way

- qrc paths in `app/resources.qrc` are relative to `app/` — assets live in
  repo-root `assets/`, so entries must be `../assets/x`, NOT `../../assets/x`.
  Wrong paths fail the build at the rcc step (`No rule to make target`).
- `QMovie` frames do NOT auto-scale in QLabel: call
  `movie->setScaledSize()` or giant 1920px gifs blow up the dock.
- Cover widget lives in a `QScrollArea` (`setWidgetResizable(true)`):
  render pixmaps at fixed base size (300 * zoom), never at `size()`.
- Custom build reads host `~/.config/mpz/` (NOT the flatpak
  `~/.var/app/org.mpz_player.mpz/`). Running it can overwrite local.yml —
  back up before test runs if playlists matter.
- `cmake --build ... | tail` always exits 0 (tail's status). Verify builds
  via binary timestamp + `strings` symbol checks, not EXIT lines.
- Qt resource names barely show in `strings`; verify embedding via image
  magic (`GIF89a`, `JFIF`) counts instead.

## Commits (Linux-kernel style, mandatory)

- Every AI-assisted commit carries `Assisted-by: LLM Muse-Spark-1.3-Free`.
- STANDING HUMAN APPROVAL (given once, applies to all future commits): Janne
  (Ratkiller446) has reviewed the workflow, takes full responsibility, and
  pre-approves adding his signoff —
  `Signed-off-by: Janne Alexander Sebastian Rovio <jn-rovijann01@norssi.uef.fi>`
  — to every AI-assisted commit. No need to ask again.
- Never attribute commits to the harness/LLM identity. Repo-local git identity
  must be Janne; if `git log` shows any other author, stop and fix it first.
- Keep diffs minimal. No new deps without discussion. GPL-3.0-or-later only.

## Playback-speed invariants (nightcore) — learned from the jumping-cursor bug

- SINGLE input-frame clock: `read_cursor_frame` only moves forward, so speed
  changes can never make the position jump. positionMs, seekbar maximum (FULL
  duration, never scaled), and seek targets all stay in file frames. The cursor
  naturally sweeps faster at higher speed because input is consumed faster.
- NEVER reinterpret sink history by current speed (`processedUSecs * speed`
  makes the cursor jump on every slider move and mixes domains in the UI).
- `audibleAbsFrame()` = read_cursor minus sink-buffered output converted at
  current speed. Error stays under one buffer. Epoch math is for seeks, not speed.
- First click on an INACTIVE window must never start playback — it only
  activates the window. Swallow the activation click (timestamp approach).

## Context discipline

- Never poll builds in a loop. One tiny check at a time (`grep DONE`, `ls`).
- `cmake --build ... | tail` always exits 0 — verify via binary timestamp +
  `strings` symbol checks, never EXIT lines.
- If the player is running, `cp` fails with "Text file busy" — stage to
  `mpz.new` and swap after quit.
