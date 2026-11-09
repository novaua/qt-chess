import QtQuick
import QtQuick.Controls

// The pieces one side has taken, grouped by type (P N B R Q) with pieces of a type overlapping,
// then "+N" when that side is ahead in material. With `wrapWidth` (the player column, BL-13) the
// groups go on to a next line where they don't fit; a group is never split.
Column {
    id: strip

    property var  pieces:    []   // piece keys in value order (ChessConnector.CapturedBy*)
    property int  advantage: 0    // material lead in pawns; shown when > 0
    property real pieceSize: 18
    property real wrapWidth: 0    // 0: one line

    readonly property real _gap:  pieceSize * 0.2
    readonly property real _step: pieceSize * 0.6   // the next piece of a group covers 40%

    readonly property var groups: {
        var result = []
        for (var i = 0; i < pieces.length; ++i) {
            var last = result[result.length - 1]
            if (last && last.key === pieces[i]) last.count++
            else result.push({ key: pieces[i], count: 1 })
        }
        return result
    }

    // The groups line by line; "+N" ends the last line (or starts its own when it doesn't fit).
    readonly property var lines: {
        var result = [[]]
        var used = 0
        for (const g of groups) {
            var w = pieceSize + _step * (g.count - 1)
            var line = result[result.length - 1]
            if (wrapWidth > 0 && line.length && used + _gap + w > wrapWidth) {
                result.push(line = [])
                used = 0
            }
            used += (line.length ? _gap : 0) + w
            line.push(g)
        }
        if (advantage > 0 && wrapWidth > 0 && used > 0
                && used + _gap + Theme.s1 + plusMetrics.advanceWidth > wrapWidth)
            result.push([])
        return result
    }

    TextMetrics {
        id: plusMetrics
        font.pixelSize: Theme.caption.pixelSize
        font.weight: Font.DemiBold
        text: "+" + strip.advantage
    }

    spacing: _gap

    Accessible.role: Accessible.StaticText
    Accessible.name: {
        var names = { p: "pawn", n: "knight", b: "bishop", r: "rook", q: "queen" }
        var parts = groups.map(g => g.count + " " + names[g.key.toLowerCase()] + (g.count > 1 ? "s" : ""))
        return (parts.length ? "Captured " + parts.join(", ") : "Nothing captured")
            + (advantage > 0 ? ", ahead by " + advantage : "")
    }

    Repeater {
        model: strip.lines

        Row {
            id: line
            required property var modelData
            required property int index
            spacing: strip._gap
            height: strip.pieceSize

            Repeater {
                model: line.modelData

                Item {
                    id: group
                    required property var modelData
                    width: strip.pieceSize + strip._step * (modelData.count - 1)
                    height: strip.pieceSize

                    Repeater {
                        model: group.modelData.count
                        Image {
                            required property int index
                            x: index * strip._step
                            width: strip.pieceSize
                            height: strip.pieceSize
                            source: Theme.piece(group.modelData.key)
                            fillMode: Image.PreserveAspectFit
                            sourceSize: Qt.size(2 * strip.pieceSize, 2 * strip.pieceSize)   // sharp when small
                        }
                    }
                }
            }

            Label {
                visible: strip.advantage > 0 && line.index === strip.lines.length - 1
                height: strip.pieceSize
                verticalAlignment: Text.AlignVCenter
                leftPadding: line.modelData.length ? Theme.s1 : 0
                text: "+" + strip.advantage
                font.pixelSize: Theme.caption.pixelSize
                font.weight: Font.DemiBold
                font.features: { "tnum": 1 }
                color: Theme.textSecondary
            }
        }
    }
}
