---
paths:
  - "**/*.{h,cpp,hpp}"
---

# C++ style

- **Object-oriented:** a class whose constructor does the work and gives its results through `const` accessors,
  helpers as private (static) members, small value types as classes with getters; not a free function returning
  a plain struct with public fields, nor helpers in an anonymous namespace (the owner's review of PR #92). Follow
  `GameChecks`, `HistoryPlayer`, `BoardDiff`: `_member` fields, short inline getters, the file named after the
  class. Existing data structs (`Move`, `PositionPiece`) stay.
- `std::shared_ptr` throughout (the `…Aptr` typedefs); no raw owning pointers.
- Chess logic stays in `ChessCore`, namespace `Chess::`, without Qt; GUI code stays in `ChessGui`.
- `PieceColors` is an `enum class` (`Empty`, `Light`, `Dark`): scoped references (`PieceColors::Light`).
- The app's own files (`users.json` and its backups, the game files, `rating.json`, `config.json`, a profile
  export) are written with `AppConfig::writeFileAtomically` (`QSaveFile`); only an import's staging folder uses
  `writeFileUnflushed`. The log is `AppLog`'s; the Settings page's options are QtCore `Settings`.
- No formatters: no clang-format, `.editorconfig` or format scripts (one was tried and reverted). Keep the style
  of the file you edit.
- A new ChessGui `.cpp` goes into `ChessGuiTests.vcxproj` too (the UI tests build every ChessGui source but
  `main.cpp`; the link fails without it).
