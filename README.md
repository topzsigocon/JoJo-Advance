# JoJo-inspired All-Star Battle GBA — Prototype package

**Target:** genuine Game Boy Advance `.gba` ROM, C++ / Butano / devkitARM.

This package is an early playable foundation, not a finished game. It contains an original placeholder-art prototype using the GBA Mode 3 framebuffer, with Butano supplying the GBA runtime and keypad/game loop.

## Included and implemented
- Butano project Makefile and C++ entry point.
- GitHub Actions workflow that runs inside the official devkitPro devkitARM container, fetches Butano, builds, verifies, and uploads the `.gba` artifact.
- Title screen.
- 50-entry roster list across Parts 1–8; the hybrid selection view displays the selected Part roster, with L/R switching Parts 1–8 and directional selection.
- Two selectable prototype fighters: Jonathan and Dio Brando. Other roster entries are listed as planned/locked slots and cannot yet be selected.
- Test arena, two abstract original placeholder fighters, health bars, simple rival AI, movement, jump, light/heavy attacks, hit checks, guard, and win state.

## Controls
### Title / selection
- START or A: enter / confirm
- D-pad: move selection
- L / R: switch between JoJo Parts 1–8
- B: go back
- Select two currently playable prototype fighters, then START to enter battle.

### Battle
- LEFT / RIGHT: move
- UP: jump
- A: light attack
- B: heavy attack
- Hold L: guard
- START: return to fighter selection
- After a winner appears, A restarts the round.

## GitHub Actions
1. Create or open your project repository.
2. Extract the contents of this ZIP into the repository root, preserving folder paths.
3. Commit and push to `main`, or run **Actions → Build JoJo All-Star GBA ROM → Run workflow**.
4. Open the completed workflow run and download the `jojo-allstar-gba` artifact.
5. Extract the artifact ZIP to obtain `jojo_allstar_gba.gba`, then open it in mGBA or another GBA emulator.

The workflow uses the `devkitPro/setup-devkitpro` action with the `gba-dev` package group and clones Butano as a sibling directory (`../butano`), matching the Makefile's `LIBBUTANO := ../butano` path.

## Local build
Install devkitPro's GBA Development component (`gba-dev`), Python 3, Git, and Make. Clone Butano next to this project directory as `../butano`, then run `make -j2` from this project's root. The output should be `jojo_allstar_gba.gba`.

## Important development notes
- The roster names are a planning list, not 50 finished fighters. Only the first two entries (Jonathan and Dio Brando) are enabled as playable prototype choices at this stage.
- Character art is generic original placeholder pixel art; no ripped sprites, logos, music, voice clips, or other extracted game assets are included.
- This is an unofficial fan-made prototype. JoJo characters and franchise references belong to their respective rights holders; do not distribute commercially or imply official endorsement. Obtain permission or replace protected references/assets before public release.
- The package has not been built in this environment. The first GitHub Actions run is the build verification step; if Butano or action inputs change upstream, inspect the workflow log and adjust accordingly.
