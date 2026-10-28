# Theme tokens

| | |
| :---- | :---- |
| **Date** | 2026-10-28 |
| **Status** | Spec for the Phase 1 `Theme.qml` singleton ([`AppModernizationPlan.md`](../AppModernizationPlan.md) §6) |

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
- High contrast: when Windows high contrast is on, use the palette's system colours and drop the
  translucent fills. This is checked in P6.

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
| `selected` | accent at 45% | the blue `Glow` in `ChessFigures.qml` (Qt5Compat, removed in P1) |
| `legalMoveDot` | `#000000` at 22% | `NavigationLayer.qml` markers |
| `checkSquare` | `critical` at 55% | `ChessBoxes.qml:51` (`#e74c3c`) |
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

There is also a "Reduce animations" setting (P5). It sets all durations to 0.

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
