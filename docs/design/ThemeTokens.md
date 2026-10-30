# Theme tokens

| | |
| :---- | :---- |
| **Date** | 2026-10-28 |
| **Status** | Implemented in P1 as `ChessGui/ChessGui/qml/Theme.qml` ([`AppModernizationPlan.md`](../AppModernizationPlan.md) §6); differences noted in §8 |

The app moves to the **FluentWinUI3** Qt Quick Controls style. Controls (buttons, switches,
sliders, text fields, menus, dialogs) take their colours from the style and the Windows theme. So
`Theme.qml` only holds what the style doesn't cover: our own surfaces, the board, game states,
spacing and the type ramp. No QML file outside `Theme.qml` hard-codes a colour.

## 1. Colour scheme

- The user's choice (System, the default, / Light / Dark) is applied by setting
  `Application.styleHints.colorScheme` (writable since Qt 6.8): `Qt.ColorScheme.Unknown` follows
  Windows, `Light` or `Dark` forces it. FluentWinUI3 controls follow that app colour scheme, so the
  controls and our own surfaces always agree.
- `Theme.dark` is simply `Application.styleHints.colorScheme === Qt.ColorScheme.Dark`.
- **Accent:** the system accent colour, via `palette.accent` from the FluentWinUI3 style. The
  fallback is `#0078D4` (light) / `#60CDFF` (dark), the WinUI defaults.
- **High contrast** (P6): when a Windows contrast theme is on
  (`Application.styleHints.accessibility.contrastPreference === Qt.HighContrast`, Qt 6.10),
  `Theme.highContrast` is true. The FluentWinUI3 controls switch to the system colours themselves;
  our roles do the same through `SystemPalette`: surfaces are `window`, text, strokes and dividers
  are `windowText`, disabled text is the disabled `windowText`, the accent is `highlight`. The
  status colours become the text colour (the InfoBar keeps its border and icon), the selected fill
  and the scrim go away (the accent border stays), and Home drops its photo. Hovered cards get an
  accent border instead of a lighter fill. The board keeps its own colours. `--high-contrast`
  (`Take-Screenshot.ps1 -HighContrast`) forces our part for a screenshot.

## 2. Colour roles

Names follow WinUI so they can be matched to the Windows design kit.

| Token | Light | Dark | Used for |
|---|---|---|---|
| `windowBg` | `#F3F3F3` | `#202020` | Page background (the Mica-like base) |
| `layerBg` | `#F9F9F9` | `#272727` | Side panel, command bar strip |
| `cardBg` | `#FFFFFF` | `#2B2B2B` | Cards: Home actions, player cards, settings cards, dialogs |
| `cardBgHover` | `#F6F6F6` | `#323232` | Hover on clickable cards |
| `cardStroke` | `#E5E5E5` | `#1D1D1D` | 1 px card border |
| `divider` | `#EAEAEA` | `#3A3A3A` | Separators, move-list row lines |
| `textPrimary` | `#1B1B1B` | `#FFFFFF` | Body and titles |
| `textSecondary` | `#5F5F5F` | `#CFCFCF` | Captions, hints, move numbers |
| `textDisabled` | `#A0A0A0` | `#787878` | Disabled labels |
| `scrim` | `#4D000000` | `#66000000` | Behind modal dialogs |
| `success` | `#0F7B0F` | `#6CCB5F` | Turn indicator, "Connected to Lichess" |
| `caution` | `#9D5D00` | `#FCE100` | Warnings, reconnect banner |
| `critical` | `#C42B1C` | `#FF99A4` | Resign, delete, check, errors |
| `criticalBg` | `#FDE7E9` | `#442726` | Destructive-confirm InfoBar |
| `homeBackdropOpacity` | `0.18` | `0.10` | The glass-pieces photo, **Home only**, under `windowBg` |

## 3. Board

The board keeps its current slate identity. These colours are the defaults today, collected from
`ChessGame.qml`, `ChessBoxes.qml` and `ChessFigures.qml`. They move into tokens so a board-theme
picker can come later (out of scope).

| Token | Value (both modes) | Today |
|---|---|---|
| `squareLight` | `#ECF0F1` | `lightChessBoxColor` |
| `squareDark` | `#2F4F4F` | `darkChessBoxColor` (`darkslategray`) |
| `lastMove` | `#A8D8EA` at 70% | `ChessBoxes.qml:40` |
| `selected` | accent at 45% | the blue `Glow` in `ChessFigures.qml` (Qt5Compat, removed in P1; now a tint on the selected piece's square) |
| `dropSquare` | accent | outline of the square under a dragged piece (`ChessBoxes.qml`, BL-04) |
| `legalMoveDot` | `#000000` at 22% | `NavigationLayer.qml` markers |
| `checkGlowCenter` / `checkGlowEdge` | `#FFD54A` / `#E5341F` | The king in check: a radial glow (yellow middle, red edge, fading corners) that pulses while in check (`ChessBoxes.qml`) |
| `flaggedKing` | `critical` | the red flag on the losing king |
| `coordOnLight` / `coordOnDark` | `#2F4F4F` / `#ECF0F1` | Coordinates drawn inside squares (option); today they sit outside the board in `#34495e` |

## 4. Type ramp

The font is **Segoe UI Variable**, falling back to Segoe UI (`QGuiApplication::setFont` already sets
Segoe UI when running offscreen). Sizes are in device-independent px, as in WinUI.

| Token | Size / line height | Weight | Used for |
|---|---|---|---|
| `caption` | 12 / 16 | Regular | Hints, timestamps, move numbers |
| `body` | 14 / 20 | Regular | Default text, move list, settings descriptions |
| `bodyStrong` | 14 / 20 | Semibold | Player names, setting titles |
| `subtitle` | 20 / 28 | Semibold | Dialog titles, card titles |
| `title` | 28 / 36 | Semibold | Page titles (Home, Settings, Profile) |
| `titleLarge` | 40 / 52 | Semibold | Welcome screen |

Moves use **tabular figures** (`font.features: { "tnum": 1 }`) so the move-list columns line up
without Courier New.

## 5. Spacing, size, radius

- **Spacing:** a 4 px grid. Tokens `s1` 4, `s2` 8, `s3` 12, `s4` 16, `s5` 24, `s6` 32, `s7` 48. Page
  margins are 24 (`s5`), card padding 16 (`s4`), and gaps between cards 8 (`s2`).
- **Radius:** controls 4 (from the style), cards and dialogs 8, avatars 6.
- **Icons:** `pics/icons/*.svg`, drawn on a 20×20 grid, shown at 16 (dense: move-list navigation), 20 (command bar, default) or 24 (Home cards). Tint them with `icon.color`; never ship coloured copies.
- **Hit targets:** at least 32×32 for icon buttons (command bar 40×40).
- **Window:** minimum size 640×560. The side panel appears at width ≥ 960 or when the aspect ratio is above 1.15. Below that, the move list becomes a bottom sheet.

## 6. Motion

| Token | Value | Used for |
|---|---|---|
| `fast` | 83 ms | Hover and pressed states |
| `normal` | 167 ms | Card and dialog fades, page transitions |
| `slow` | 250 ms | Board flip (replaces today's elastic easing), side-panel slide |
| easing | `Easing.OutCubic` | Everything, unless the style animates it itself |

There is also a "Reduce animations" setting (P5). It sets all durations to 0. Since P6
`Theme.reduceMotion` is also on when Windows' animation effects are off
(`AppInfo.windowsAnimationsOff`; Settings then shows the switch on and disabled), and in
screenshot mode (`Theme.forceReduceMotion`, so pictures repeat).

**Page transitions** (P6, `StackView` in `ChessGame.qml`): the new page fades in over `normal` and
rises `s5` (24 px), the old one fades out over `fast`; Back does the reverse. They replace the
Basic style's 400 ms horizontal slide, and a disabled `Transition` switches pages at once.

## 7. Sketch of `Theme.qml` (P1)

```qml
pragma Singleton
import QtQuick

QtObject {
    // 0 System, 1 Light, 2 Dark; persisted in Settings. Forcing a scheme also restyles the controls.
    property int scheme: 0
    onSchemeChanged: Application.styleHints.colorScheme =
        [Qt.ColorScheme.Unknown, Qt.ColorScheme.Light, Qt.ColorScheme.Dark][scheme]
    readonly property bool dark: Application.styleHints.colorScheme === Qt.ColorScheme.Dark

    readonly property color windowBg:    dark ? "#202020" : "#F3F3F3"
    readonly property color cardBg:      dark ? "#2B2B2B" : "#FFFFFF"
    readonly property color textPrimary: dark ? "#FFFFFF" : "#1B1B1B"
    // ... the rest of the roles from §2–§3

    readonly property int s2: 8
    readonly property int s4: 16
    // ...
}
```

It is registered through `qmldir` (`singleton Theme 1.0 Theme.qml`) and listed in `qml.qrc`. Every
`readonly property bool isDarkMode: { … Window.window.color … }` block (15 files today) becomes
`Theme.dark`.

## 8. As implemented (P1)

- **Setting:** `appSettings.theme` in `ChessGame.qml` (0 System, 1 Light, 2 Dark). While it is unset
  (-1), the old `darkMode` bool decides (Dark or System), so existing users keep their choice.
  `ChessGame.qml` binds `Theme.scheme` to it and `Theme.systemAccent` to the window's
  `palette.accent`; Settings shows the choice as "App theme".
- **Extra tokens:** `selectedBg` (accent at 15% light / 25% dark: selected avatar, current level,
  reviewed move), `textOnColor` (`#FFFFFF` on saturated fills such as the level badges),
  `iconSize` 20 / `iconSizeDense` 16, `radiusCard` / `radiusAvatar`, and `Theme.icon(name)` →
  `qrc:/icons/<name>.svg`.
- **Board:** `flaggedKing` uses a fixed red (`#C42B1C`, the light `critical`) in both modes; the
  dark `critical` pink looked washed out on the board. The check glow replaced the flat
  `checkSquare` tint: red-yellow, so it reads as "danger" on both square colours.
- **Type ramp:** the tokens are `font` values (`font: Theme.bodyStrong`) with size and weight only;
  the family stays the app font.
- **Colour scheme and `-platform offscreen`:** the offscreen platform can't force a colour scheme
  (it stays Unknown) and FluentWinUI3 then uses its dark palette, so screenshots use a transparent
  real window instead (`run-chess-app` skill).

## 9. Added in P2

- `successBg` (`#DFF6DD` / `#393D1B`) and `cautionBg` (`#FFF4CE` / `#433519`): InfoBar backgrounds,
  from WinUI; `criticalBg` is reused for error InfoBars and `cardBg` for informational ones.
- `iconSizeCard` 24 (Home cards, the account avatar) and `commandBarHeight` 48 (both page headers;
  the InfoBars sit just below it).

## 10. Added in P4

- Game page sizes: `playerCardHeight` 60, `sidePanelWidth` 280 (the move list beside the board when
  the window is at least 960 wide or wider than 1.15 × its height) and `sheetHandleHeight` 44 (the
  collapsed move sheet otherwise).
- `Theme.piece(key)`: the board's piece image for a key, `"N"` white knight, `"n"` black knight.
  Captured pieces keep their colour. Move-list figurines use the text colour instead (white images
  in dark mode, black in light), because black pieces disappear on a dark panel.
- Turn indicator: an `accent` bar on the player card's left edge, plus the avatar pulse (`accent`
  at 20 %). In P2 it was `success`.
- `-platform offscreen` (screenshots on a locked screen) sets `Theme.dark`: the Fluent style is dark
  there and can't be switched.

## 11. Added in P6

- `highContrast`, `forceHighContrast` and the system colours behind every role (§1).
- `focusStroke` (= `textPrimary`): the keyboard focus outline that `FocusRing.qml` draws around
  items the style doesn't, such as the selected avatar or opponent when the group has focus.
- `forceReduceMotion` (screenshots) and `AppInfo.windowsAnimationsOff` feed `reduceMotion` (§6).
- App icon: the board's white knight on a rounded tile in the board's green with a 2×2 board
  corner (`pics/app-icon.png`, `chess.ico`, installer images in `installer/art/`), drawn by
  `scripts/make_icons.py`. It replaces `pics/chess_icon.svg`.
