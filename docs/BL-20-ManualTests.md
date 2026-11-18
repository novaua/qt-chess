# BL-20 Signed Game Files — Manual Tests

| | |
| :---- | :---- |
| **Date** | 2026-11-19 |
| **Build** | `develop` with #188–#190 (Release) |
| **Plan** | [`BL-20-GameFilesPlan.md`](BL-20-GameFilesPlan.md) (§5: "By hand") |

The automated tests edit the files the way a test can: a byte replaced, a file written by the test itself.
Only you, with a text editor, show what a player would actually try. These tests check four things:
- the app notices each edit and says so once;
- nothing else is lost;
- a profile moves between data folders and keeps its games, rating and statistics;
- your own data from before BL-20 is updated without a message.

Write the result in the last line of each scenario: **OK**, or what happened instead. Add the time, so the
app log can be found: *Settings → About → Open log folder*. Every file the app sets aside has a
`GameLibrary: … set aside: <why>` line there, and every restore has a `restored` line.

**Use throwaway data folders** for scenarios 1–6, so your own profiles and games stay as they are. Start
the app from a terminal with its own folder:

```powershell
$app  = "$env:ProgramFiles\Chess++\ChessGui.exe"   # or x64\Release\ChessGui.exe of a build
$data = "$env:TEMP\chess-bl20"
& $app --data-dir $data
```

**The files** (*Settings → About → Data folder* opens the folder):
- `users.json`: the profiles, each with its `id` and its `"stats"` (with `"statsSig"`).
- `users\<profile id>\games\<game id>.json`: one file per game, `{"record": {…}, "sig": "1:…"}`. The game you
  played last is the file changed last (sort the folder by date).
- `users\<profile id>\rating.json`: `{"rating":{"games":2,"value":1252},"sig":"1:…","version":1}`.
- `users\<profile id>\games\rejected\`: where files that fail their check are moved (never deleted).
- `backups\users-1.json` … `users-3.json`: the last three copies of `users.json`, made when the app starts.

**A rated game**: vs the robot, standard (not Chess960), started from the setup dialog (not Play from
here), no takeback, and both sides moved before it ended (resigning after one move each is enough). Its
card says "You won · +27" or similar.

**Several messages at once**: the last one stays on screen; the earlier ones are in *Recent messages*
(Ctrl+M).

Already checked by the tests, no need to look again:
- in ChessQtTests:
  - a game edited, unsigned, not JSON, or copied under another game's name is set aside;
  - `games.json` is migrated, with `ratedGames` worked out;
  - the rating is restored from the newest rated game, or starts again;
  - a rating that is behind its newest game catches up;
  - statistics come back from the newest backup that verifies, or are rebuilt from the games of both profiles;
  - a `users.json` from before BL-20 is trusted once;
  - an export round trip verifies;
  - an old export imports unrated, and an edited export has its failing parts set aside;
- in `tst_restart`: an edited game and an edited `rating.json` between two runs, and their messages.

## 1. A game edited by hand is set aside

1. Start a new profile in `$data`. Play a game vs the robot and resign after a few moves. Start a two-player
   game, play two moves and go Home (it stays unfinished). Close the app.
2. In `users\<id>\games\`, open the robot game's file (the one with `"kind":"computer"`) in Notepad and change
   `"result":"0-1"` to `"result":"1-0"` (or `"1-0"` to `"0-1"` if you played Black). Save. Start the app.
   - **Expected**: "1 kept game couldn't be verified and was set aside." The Games page lists only the
     two-player game. The robot game's file is in `games\rejected\`, unchanged. The log says why
     ("its signature doesn't fit").
3. Close and start the app again.
   - **Expected**: no message this time. The two-player game can still be continued.

Result:

## 2. A damaged game file, and one copied under another name

1. In the same data folder, finish the two-player game (resign), and play one more two-player game to
   the end. Close the app.
2. Open one game file in Notepad and delete its last few characters (the file is no longer valid JSON). Copy
   the other game file in the same folder and name the copy `copy-1.json`. Start the app.
   - **Expected**: "2 kept games couldn't be verified and were set aside." Both the damaged file and
     `copy-1.json` are in `games\rejected\`. The copied game's original is still listed: copying a game
     doesn't make two of it. The log says "not JSON" and "another game's id".

Result:

## 3. The rating edited by hand

1. In a new profile (or the same folder), play two **rated** games vs the robot (see above). Note the rating
   on Home's header and on the Profile page, and how many rated games it says. Close the app.
2. Open `users\<id>\rating.json` in Notepad and change the `value` to `2400`. Save. Start the app.
   - **Expected**: "Your rating couldn't be verified and was restored from your kept games." The rating
     and the number of rated games are what you noted, not 2400. The edited file is in `games\rejected\`
     (as `rating.json`). The game cards still show their own rating changes.
3. Close the app, delete `rating.json` and start again.
   - **Expected**: the same message, the same rating.
4. Close, start again.
   - **Expected**: no message.

Result:

## 4. The statistics edited by hand

1. In the profile of scenario 3, look at the Profile page's statistics (the robot's rows). Close the app,
   start it once and close it again: the start copies `users.json` to `backups\users-1.json` with these
   numbers.
2. Open `users.json` in Notepad. Under your profile's `"stats"` → `"byOpponent"`, raise a robot row's
   `"wins"` and `"games"` by 10. Leave `"statsSig"` as it is. Save. Start the app.
   - **Expected**: "<your name>'s statistics couldn't be verified and were restored from a backup." The
     statistics are the ones from step 1, not the edited ones.
3. Close the app. Delete the `backups` folder, edit `users.json` the same way again, and start the app.
   - **Expected**: "<your name>'s statistics couldn't be verified and were rebuilt from the kept games
     (Lichess and older games aren't in them)." The statistics count only the games the Games page lists
     (minus those set aside in scenarios 1–2).
4. Close, start again.
   - **Expected**: no message; the statistics stay as restored.

Result:

## 5. Export and import between two data folders

1. In `$data`, use a profile with a few kept games, a rating vs the robot and statistics. Note the rating,
   the number of rated games, the statistics totals and the number of games on the Games page. On the
   Profile page: **Export profile…**, and save the `.chessprofile` somewhere outside `$data`. Close the app.
2. Start the app on a second, empty folder (`& $app --data-dir "$env:TEMP\chess-bl20-other"`), create any
   profile, then on Home's account menu: **Switch user** → **Import profile…**, and choose the file.
   - **Expected**: "Profile imported", with nothing more. Sign in as the imported profile: its games, rating,
     rated-games count and statistics are the ones you noted. A finished game opens. An unfinished one
     continues.
3. Close and start the app again on that folder, signed in as the imported profile.
   - **Expected**: no message.

Result:

## 6. An export edited by hand

1. Copy the `.chessprofile` of scenario 5. Open the copy in Notepad: `"profile"` is plain JSON. In its
   `"stats"` → `"byOpponent"`, raise a row's `"wins"` and `"games"`. Save. (The games and the rating are
   inside `"files"`, base64-encoded, so editing them is left to the automated tests.)
2. Import the copy into the second folder: **Import as new** when it asks.
   - **Expected**: "Profile imported. Its statistics couldn't be verified and were rebuilt from its kept
     games." The new profile ("Name (2)") has its games and its rating as exported, and statistics counted
     from its games.

Result:

## 7. Your own data from before BL-20, updated

This needs a data folder last used by a build **before #188**, such as the installed release or a build of
`develop` before 2026-11-18. Copy that folder; never use the original:

```powershell
$old = "$env:LOCALAPPDATA\ChessPlusPlus_Inc\ChessPlusPlus"   # the folder with users.json (Settings → About → Data folder)
Copy-Item $old "$env:TEMP\chess-bl20-mine" -Recurse
& $app --data-dir "$env:TEMP\chess-bl20-mine"
```

1. Start the new build on the copy. Sign in to each profile in turn.
   - **Expected**: no "couldn't be verified" message for any profile. Every game the old build listed is on
     the Games page. The rating and rated-games count are as before, and so are the statistics. Each profile
     folder has `games\` with one file per game, `rating.json` (once a game was rated), and
     `games.json.migrated` (the old file, kept).
2. Play one game to the end, then close the app. Open `users.json`.
   - **Expected**: `"version": 3`, and every profile has a `"statsSig"`.
3. Start the app again.
   - **Expected**: no message; everything as in step 1, plus the new game.

If the only data folder you have was already opened by a build with #188, skip this and write "no old data".

Result:

## 8. Optional: another Windows account, a reinstall

- **Another Windows account** (if you have one): copy the data folder of scenario 5 into that account's
  `%LOCALAPPDATA%\ChessPlusPlus_Inc\ChessPlusPlus` (or use `--data-dir` on the copy), and start the app there.
  - **Expected**: games, rating and statistics verify, with no message. A profile linked to Lichess shows
    "reconnect" (its sign-in can't be decrypted by another account, #187).
- **A reinstall**: uninstall Chess++ and answer **No** to deleting the user data, then install the same
  build again.
  - **Expected**: every profile's games, rating and statistics as before, with no message.

Result:
