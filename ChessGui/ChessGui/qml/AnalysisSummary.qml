import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ChessPlusPlus

// The analysis of the game on the board (BL-19 step 7), in the moves panel: Analyse game when it can be
// analysed, the progress while it is (Cancel), then each side's accuracy and its inaccuracies (?!),
// mistakes (?) and blunders (??), and what the move on the board was ("Blunder · Best was Nf3").
ColumnLayout {
    id: summary

    readonly property var analysis: ChessConnector.Analysis
    readonly property var shown: ChessConnector.ShownAnalysis
    readonly property bool analysed: analysis.depth !== undefined
    readonly property bool engineFound: AppInfo.engineFound
    // Not found at start: looked for again with each game page (BL-25 review; nothing once found).
    Component.onCompleted: AppInfo.probeEngine()

    signal analyseRequested()   // GamePage.analyse(): also closes the result card

    visible: ChessConnector.AnalysingThisGame || analysed || ChessConnector.CanAnalyse
    spacing: Theme.s1

    // Not analysed yet: centred, as the review buttons under it.
    Item {
        Layout.fillWidth: true
        implicitHeight: analyseButton.implicitHeight
        visible: !summary.analysed && !ChessConnector.AnalysingThisGame
        Button {
            id: analyseButton
            objectName: "analyseButton"
            anchors.horizontalCenter: parent.horizontalCenter
            enabled: summary.engineFound
            text: "Analyse game"
            icon.source: Theme.icon("analyse")
            ToolTip.visible: hovered && !enabled
            ToolTip.text: AppInfo.engineState === "checking" ? "Looking for the chess engine…"
                                                              : "The analysis needs the chess engine (Settings → Computer)."
            onClicked: summary.analyseRequested()
        }
    }

    // Under way.
    RowLayout {
        objectName: "analysisProgress"
        Layout.alignment: Qt.AlignHCenter
        visible: ChessConnector.AnalysingThisGame
        spacing: Theme.s2
        BusyRing {}
        Label {
            text: "Analysing… " + ChessConnector.AnalysisDone + " of " + ChessConnector.AnalysisTotal
            font.pixelSize: Theme.body.pixelSize
            font.features: { "tnum": 1 }
        }
        Button {
            objectName: "cancelAnalysisButton"
            text: "Cancel"
            onClicked: ChessConnector.cancelAnalysis()
        }
    }

    // Done: a row per side.
    component SideRow: RowLayout {
        id: sideRow
        property bool white
        readonly property var side: summary.analysed ? (white ? summary.analysis.white : summary.analysis.black) : ({})
        readonly property string player: white === ChessConnector.PlayerPlaysWhite ? ChessConnector.PlayerName
                                                                                    : ChessConnector.OpponentName
        Layout.fillWidth: true
        spacing: Theme.s2
        Rectangle {
            width: 12; height: 12; radius: 6
            color: sideRow.white ? "#F2F2F2" : "#303030"
            border.color: Theme.textSecondary
        }
        Label {
            Layout.fillWidth: true
            text: sideRow.player
            font: Theme.body
            elide: Text.ElideRight
        }
        Label {
            text: sideRow.side.accuracy >= 0 ? sideRow.side.accuracy + "%" : "–"
            font.pixelSize: Theme.bodyStrong.pixelSize
            font.weight: Theme.bodyStrong.weight
            font.features: { "tnum": 1 }
            Accessible.name: "Accuracy " + text
            ToolTip.visible: accuracyHover.hovered
            ToolTip.text: "Accuracy"
            HoverHandler { id: accuracyHover }
        }
        Repeater {
            model: [
                { mark: "?!", count: sideRow.side.inaccuracies, name: "inaccuracies" },
                { mark: "?",  count: sideRow.side.mistakes,     name: "mistakes" },
                { mark: "??", count: sideRow.side.blunders,     name: "blunders" },
            ]
            Label {
                required property var modelData
                Layout.preferredWidth: 34
                horizontalAlignment: Text.AlignRight
                text: (modelData.count ?? 0) + " " + modelData.mark
                font.pixelSize: Theme.caption.pixelSize
                font.weight: Font.DemiBold
                font.features: { "tnum": 1 }
                color: modelData.count > 0 ? Theme.markColor(modelData.mark) : Theme.textSecondary
                Accessible.name: (modelData.count ?? 0) + " " + modelData.name
            }
        }
    }

    ColumnLayout {
        objectName: "analysisSummary"
        Layout.fillWidth: true
        visible: summary.analysed
        spacing: 2

        RowLayout {
            Layout.fillWidth: true
            Label {
                Layout.fillWidth: true
                text: "Analysis"
                font: Theme.bodyStrong
            }
            Label {
                objectName: "shownEval"
                text: summary.shown.evalText ?? ""
                font.pixelSize: Theme.bodyStrong.pixelSize
                font.weight: Theme.bodyStrong.weight
                font.features: { "tnum": 1 }
                Accessible.name: "Evaluation " + text
            }
        }
        SideRow { white: true }
        SideRow { white: false }
        // The move on the board, when it was judged; "Best was …" green, as its arrow on the board.
        Label {
            objectName: "shownJudgement"
            Layout.fillWidth: true
            visible: (summary.shown.judgement ?? "") !== ""
            textFormat: Text.StyledText
            text: (summary.shown.judgement ?? "")
                  + (summary.shown.best ? " · <font color='" + Theme.success + "'>Best was " + summary.shown.best + "</font>" : "")
            font: Theme.caption
            color: Theme.markColor(summary.shown.mark ?? "")
            wrapMode: Text.WordWrap
        }
    }
}
