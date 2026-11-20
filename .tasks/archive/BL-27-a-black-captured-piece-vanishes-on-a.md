---
id: BL-27
title: "A black piece in the captured strip nearly vanishes on a dark card"
type: bug
status: done
priority: low
---

# BL-27 A black piece in the captured strip nearly vanishes on a dark card

Found in BL-19's hand test 9.2 (2026-11-20, the owner's screenshot): in Windows' contrast theme (a black window), the
player card's captured strip showed only the outline of the black knight Mandanus had taken; the black pawns beside it
weren't visible at all. Dark mode (card `#2B2B2B`) likely has the same problem, only milder.

**Cause:** `CapturedStrip.qml` draws each piece as a plain `Image` (`Theme.piece`). The move list's figures
(`MoveList.qml`) have a glow in the other colour (`Theme.pieceGlowOnBlack` / `pieceGlowOnWhite`, a `MultiEffect` shadow)
that keeps black pieces visible on a dark panel; the strip has none.

**Fix idea:** the same glow in the strip (one `layer.effect` per group or per strip, not per piece, since the strip
can hold 15 pieces), at least in dark mode and high contrast. Note: `layer.effect` figures don't draw in the offscreen UI
tests (see the move list); check with the `capture` scenes in `--high-contrast` and dark.

**See it:** a game where White has taken a black piece; Settings → Appearance → Dark, or a Windows contrast theme;
look at White's player card.

## Resolution

### 2026-11-20 — fixed (branch: bugs/capturedStripGlow_201126)

- **Cause:** as above: the strip's pieces had no glow; a black piece on a dark card (dark mode `#2B2B2B`, a contrast theme's black) all but vanished.
- **Fix:** `CapturedStrip.qml`: the move list's glow (`MultiEffect` shadow, `Theme.pieceGlowOnBlack` / `pieceGlowOnWhite`), one layer per group (its pieces share a colour). Both glows are on only where shader effects run (`GraphicsInfo.api !== GraphicsInfo.Software`): `-platform offscreen` renders with Qt Quick's software renderer (checked: api 1 offscreen, 4 = D3D11 in a window), where an item with `layer.effect` isn't drawn at all. So the move list's figures now draw in the offscreen UI tests and `-Offscreen` screenshots too (plain); the rule is in CLAUDE.md's Key Conventions.
- **Checked:** the `captures` scene light, dark, high contrast (dark) and offscreen: black pieces glow white on the dark card, white ones get a faint dark edge on the light one, offscreen plain; the `analysis` scene offscreen: the move list's figures drawn. UI tests (Release): 13 files, 0 failed.
