#define Position struct pos
struct pos{
    int x;
    int y;
};

bool whitePawnValidity(Position oldPosition, Position  newPosition,int chessBoard[8][8]);
bool blackPawnValidity(Position oldPosition, Position  newPosition, int chessBoard[8][8]);
bool RookValidity(Position oldPosition, Position  newPosition, int chessBoard[8][8]);
bool BishopValidity(Position oldPosition, Position  newPosition, int chessBoard[8][8]);
bool KnightValidity(Position oldPosition, Position  newPosition, int chessBoard[8][8]);
bool QueenValidity(Position oldPosition, Position  newPosition, int chessBoard[8][8]);
bool KingValidity(Position oldPosition, Position  newPosition, int chessBoard[8][8]);