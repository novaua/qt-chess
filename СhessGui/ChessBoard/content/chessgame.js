
var maxIndex = 64;
var board = new Array(maxIndex);
var pieceSrc = "Piece.qml"
var component = Qt.createComponent(blockSrc);
var gameCanvas;

function cleanUp()
{
    if (gameCanvas == undefined)
        return;

    for (var i = 0; i < maxIndex; i++) {
        if (board[i] != null)
            board[i].destroy();
        board[i] = null;
    }
}

function startNewGame(gc, mode, map)
{
    cleanUp();
}

function feeBoard()
{
    if (gameCanvas == undefined)
        return;

    for (var i = 0; i < maxIndex; i++) {
        if (board[i] != null)
            board[i].destroy();
        board[i] = null;
    }
}
