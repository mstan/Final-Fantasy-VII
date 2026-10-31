# Automatic disc switching

Automatic Disc Switching is enabled by default. It can be disabled in the
launcher Mods page. Configure all three original NTSC-U images in the existing
disc roster. Manual selection is available in the game menu (Ctrl+L / F10 or
Select+L3), including recovery when a configured image cannot be opened.

The game-owned plugin reports the real requested disc and waits until the
original disc-change routine has stopped using the previous image. The shared
framework performs its normal lid-open/close cycle at VBlank. The game still
validates the new disc and resumes itself. There is no pixel matching or guest
RAM patch. Automatic attempts do not repeat during the same request after a
failure; netplay disables this feature.

## Clean-room evidence

Derived directly from owned NTSC-U executable/DSCHANGE.X bytes:

- `FIELD/DSCHANGE.X` is an uncompressed 6004-byte overlay at `0x800A0000`,
  identical on all three discs (SHA256
  `a41a8565b8747daf8cfecb143974de47d91dbcbe75abca3fedc205e45e3de868`).
- Main at `0x80011CC0` establishes `$s6 = 0x8009D588`. The call sites
  `0x80011DF8..0x80011E00` (load) and `0x800124B4..0x800124BC` (gameplay)
  pass its byte to DSCHANGE. It is the requested disc, not mounted media.
- DSCHANGE preserves the argument in `$fp`. It loads the disc-number graphic
  and background, finishes its fade, then calls `0x80034104` at `0x800A077C`.
- `0x80034104` issues CD commands, waits, stops the drive, and publishes
  CD state 7 at `0x80071A60`. DSCHANGE then polls status bit `0x10` (ShellOpen)
  at `0x800A08A4..0x800A08B8`, followed by the lid-close loop.
- After media initialization, `0x800A0C04` calls `0x800343F0` to identify the
  disc and compares its result with `$fp` at `0x800A0C0C`. Identification reads
  `MINT/DISKINFO.CNF`. The plugin leaves this validation untouched.
- The detector verifies instructions in the overlay prologue, lid predicate,
  and requested-disc comparison. Other overlays reuse the address range.
  CD state 7 gates insertion until prompt assets and the fade have completed.

No external decompilation source was consulted. Original assets are not included.
Validated in isolated headless runs using copied raw memory cards, with the
mod enabled by its manifest default and no manual disc commands or RAM writes:

| Mounted at boot | Loaded save | Automatic change | Result |
| --- | --- | --- | --- |
| Disc 1 | FILE07, Woodlands Area | 1 -> 2 | Resumed field |
| Disc 1 | FILE10 | 1 -> 3 | Resumed Highwind field |
| Disc 2 | FILE01, Sector 5 Slum | 2 -> 1 | Resumed field |

Each run recorded exactly one successful `origin=auto` event in
`disc_tray_log`; boot/title/menu navigation produced none. Screenshots and
rings are in the local `Z:/psxrecomp-staging/ff7-disc-qol-20261012` validation
folders. The shared tray unit test additionally covers missing media,
readiness transitions without retries, manual recovery, an already-mounted
disc, and netplay refusal. The full title build passed without changing its
generated game code. No personal save was modified.

Tracks `beads-z9df.2`; shared framework service `beads-eio.3.342`.
