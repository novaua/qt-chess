import QtQuick
import QtQuick.Controls

// The pieces one side has taken, grouped by type (P N B R Q) with pieces of a type overlapping,
// then "+N" when that side is ahead in material.
Row {
    id: strip

    property var  pieces:    []   // piece keys in value order (ChessConnector.CapturedBy*)
    property int  advantage: 0    // material lead in pawns; shown when > 0
    property real pieceSize: 18

    readonly property var groups: {
        var result = []
        for (var i = 0; i < pieces.length; ++i) {
            var last = result[result.length - 1]
            if (last && last.key === pieces[i]) last.count++
            else result.push({ key: pieces[i], count: 1 })
        }
        return result
    }

    spacing: pieceSize * 0.2
    height: pieceSize

    Accessible.role: Accessible.StaticText
    Accessible.name: {
        var names = { p: "pawn", n: "knight", b: "bishop", r: "rook", q: "queen" }
        var parts = groups.map(g => g.count + " " + names[g.key.toLowerCase()] + (g.count > 1 ? "s" : ""))
        return (parts.length ? "Captured " + parts.join(", ") : "Nothing captured")
            + (advantage > 0 ? ", ahead by " + advantage : "")
    }

    Repeater {
        model: strip.groups

        Item {
            id: group
            required property var modelData
            readonly property real step: strip.pieceSize * 0.6   // the next piece covers 40%
            width: strip.pieceSize + step * (modelData.count - 1)
            height: strip.pieceSize

            Repeater {
                model: group.modelData.count
                Image {
                    required property int index
                    x: index * group.step
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
        visible: strip.advantage > 0
        height: strip.pieceSize
        verticalAlignment: Text.AlignVCenter
        leftPadding: strip.groups.length ? Theme.s1 : 0
        text: "+" + strip.advantage
        font.pixelSize: Theme.caption.pixelSize
        font.weight: Font.DemiBold
        font.features: { "tnum": 1 }
        color: Theme.textSecondary
    }
}
