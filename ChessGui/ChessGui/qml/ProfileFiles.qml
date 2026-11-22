import QtQuick
import QtQuick.Dialogs
import QtCore
import ChessPlusPlus

// A profile's files (BL-35): export, import (with ImportConflictDialog when the profile is here already) and
// delete, with their dialogs. One instance in the window, `profileFiles`; the pages ask through the window
// (ProfilePage, ProfilePickerPage, SettingsPage, WelcomePage). What comes of it is a short message.
Item {
    id: files
    visible: false

    signal message(string text, string severity, bool external)
    signal closeProfileRequested()   // the active profile is deleted: the Profile page goes first

    // One of its dialogs is open: the window's shortcuts wait.
    readonly property bool dialogOpen: exportDialog.visible || importDialog.visible
                                       || conflictDialog.visible || deleteConfirm.visible

    function exportProfile(userId) {
        exportDialog.userId = userId
        // Encoded: a '#' or '%' in a name is not URL syntax.
        exportDialog.selectedFile = exportDialog.currentFolder + "/"
                                  + encodeURIComponent(UserManager.exportFileName(userId))
        exportDialog.open()
    }
    function openImport() {
        importDialog.open()
    }
    function inspectImport(file) {
        const info = UserManager.inspectImport(file)
        if (info.error !== "") {
            files.message(info.error, "critical", false)
        } else if (info.conflictId !== "") {
            conflictDialog.file = file
            conflictDialog.importName = info.name
            conflictDialog.existingName = info.conflictName
            conflictDialog.open()
        } else {
            files._importProfile(file, false)
        }
    }
    function _importProfile(file, replace) {
        const error = UserManager.importProfile(file, replace)
        if (error !== "") {
            files.message(error, "critical", false)
            return
        }
        // BL-20 step 3: what the import's check found, in the same message.
        const check = UserManager.lastImport()
        const notes = []
        if (check.unrated)
            notes.push("It comes from an older Chess++: its games are unrated, its rating starts at 1200"
                       + (check.statsRebuilt ? " and its statistics count only its kept games." : "."))
        else if (check.statsRebuilt)
            notes.push("Its statistics couldn't be verified and were rebuilt from its kept games.")
        if (check.ratingRestored)
            notes.push("Its rating couldn't be verified and was restored from its games.")
        if (check.setAside > 0)
            notes.push((check.setAside === 1 ? "1 game couldn't be verified and was"
                                             : check.setAside + " games couldn't be verified and were")
                       + " set aside.")
        if (notes.length === 0)
            files.message("Profile imported", "success", false)
        else
            files.message("Profile imported. " + notes.join(" "), "caution", true)
    }
    function deleteProfile(userId) {
        const user = UserManager.users.find(u => u.id === userId)
        if (!user) return
        deleteConfirm.userId = userId
        deleteConfirm.title = "Delete " + user.name + "'s profile?"
        deleteConfirm.open()
    }

    FileDialog {
        id: exportDialog
        property string userId
        title: "Export profile"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Chess++ profiles (*.chessprofile)", "All files (*)"]
        defaultSuffix: "chessprofile"
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: {
            const error = UserManager.exportProfile(userId, selectedFile)
            if (error !== "") files.message(error, "critical", false)
            else              files.message("Profile exported", "success", false)
        }
    }

    FileDialog {
        id: importDialog
        title: "Import profile"
        fileMode: FileDialog.OpenFile
        nameFilters: ["Chess++ profiles (*.chessprofile)", "All files (*)"]
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: files.inspectImport(selectedFile)
    }

    ImportConflictDialog {
        id: conflictDialog
        objectName: "importConflictDialog"
        property url file
        onImportAsNew:      files._importProfile(file, false)
        onReplaceExisting:  files._importProfile(file, true)
    }

    ConfirmDialog {
        id: deleteConfirm
        objectName: "deleteProfileConfirm"
        property string userId
        text: "Its statistics and games are deleted too. This can't be undone; export the profile first to keep a copy."
        acceptText: "Delete"
        onAccepted: {
            if (userId === UserManager.activeUserId)
                files.closeProfileRequested()
            UserManager.deleteUser(userId)
        }
    }
}
