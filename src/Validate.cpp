#include<bits/stdc++.h>
#include "validate.h"

bool whitePawnValidity(Position oldPosition, Position newPosition, int chessBoard[8][8]){
    if(newPosition.y == oldPosition.y && newPosition.x == oldPosition.x-1){
        if(chessBoard[newPosition.x][newPosition.y]) return false;
        return true;
    }

    if(newPosition.y == oldPosition.y && newPosition.x == oldPosition.x-2){
        if(oldPosition.x != 6) return false;
        if(chessBoard[newPosition.x-1][newPosition.y] || chessBoard[newPosition.x][newPosition.y]) return false;
        return true;
    }

    if(newPosition.x == oldPosition.x-1 && newPosition.y == oldPosition.y+1){
        if(chessBoard[newPosition.x][newPosition.y] < 0) return true;
        return false;
    }

    if(newPosition.x == oldPosition.x-1 && newPosition.y == oldPosition.y-1){
        if(chessBoard[newPosition.x][newPosition.y] < 0) return true;
        return false;
    }

    return false;
}

bool blackPawnValidity(Position oldPosition, Position newPosition, int chessBoard[8][8]){
    if(newPosition.y == oldPosition.y && newPosition.x == oldPosition.x+1){
        if(chessBoard[newPosition.x][newPosition.y]) return false;
        return true;
    }
    
    if(newPosition.y == oldPosition.y && newPosition.x == oldPosition.x+2){
        if(oldPosition.x != 1) return false;
        if(chessBoard[newPosition.x-1][newPosition.y] || chessBoard[newPosition.x][newPosition.y]) return false;
        return true;
    }

    if(newPosition.x == oldPosition.x+1 && newPosition.y == oldPosition.y+1){
        if(chessBoard[newPosition.x][newPosition.y] > 0) return true;
        return false;
    }

    if(newPosition.x == oldPosition.x+1 && newPosition.y == oldPosition.y-1){
        if(chessBoard[newPosition.x][newPosition.y] > 0) return true;
        return false; 
    }

    return false;
}

bool RookValidity(Position oldPosition, Position newPosition, int chessBoard[8][8]){
    if(newPosition.x == oldPosition.x){
        if(oldPosition.y < newPosition.y){
            for(int c=oldPosition.y+1; c<newPosition.y; ++c)
                if(chessBoard[oldPosition.x][c]) return false;
        }

        else{
            for(int c=oldPosition.y-1; c>newPosition.y; --c)
                if(chessBoard[oldPosition.x][c]) return false;
        }
        return true;
    }

    if(newPosition.y == oldPosition.y){
        if(oldPosition.x < newPosition.x){
            for(int r=oldPosition.x+1; r<newPosition.x; ++r)
                if(chessBoard[r][oldPosition.y]) return false;
        }

        else{
            for(int r=oldPosition.x-1; r>newPosition.x; --r)
                if(chessBoard[r][oldPosition.y]) return false;
        }
        return true;
    }

    return false;
}

bool BishopValidity(Position oldPosition, Position newPosition, int chessBoard[8][8]){
    if(abs(newPosition.x - oldPosition.x) != abs(newPosition.y - oldPosition.y)) return false;

    if(oldPosition.x < newPosition.x && oldPosition.y < newPosition.y){
        for(int r=oldPosition.x+1,c=oldPosition.y+1; r<newPosition.x; ++r,++c)
            if(chessBoard[r][c]) return false;
    }

    if(oldPosition.x < newPosition.x && oldPosition.y > newPosition.y){
        for(int r=oldPosition.x+1,c=oldPosition.y-1; r<newPosition.x; ++r,--c)
            if(chessBoard[r][c]) return false;
    }

    if(oldPosition.x > newPosition.x && oldPosition.y < newPosition.y){
        for(int r=oldPosition.x-1,c=oldPosition.y+1; r>newPosition.x; --r,++c)
            if(chessBoard[r][c]) return false;
    }

    if(oldPosition.x > newPosition.x && oldPosition.y > newPosition.y){
        for(int r=oldPosition.x-1,c=oldPosition.y-1; r>newPosition.x; --r,--c)
            if(chessBoard[r][c]) return false;
    }

    return true;
}

bool KnightValidity(Position oldPosition, Position newPosition,int chessBoard[8][8]){
    if(chessBoard[oldPosition.x][oldPosition.y]==0) return false;
    if(newPosition.x == oldPosition.x-1 && newPosition.y == oldPosition.y-2) return true;
    if(newPosition.x == oldPosition.x-1 && newPosition.y == oldPosition.y+2) return true;
    if(newPosition.x == oldPosition.x+1 && newPosition.y == oldPosition.y+2) return true;
    if(newPosition.x == oldPosition.x+1 && newPosition.y == oldPosition.y-2) return true;
    if(newPosition.x == oldPosition.x-2 && newPosition.y == oldPosition.y+1) return true;
    if(newPosition.x == oldPosition.x-2 && newPosition.y == oldPosition.y-1) return true;
    if(newPosition.x == oldPosition.x+2 && newPosition.y == oldPosition.y+1) return true;
    if(newPosition.x == oldPosition.x+2 && newPosition.y == oldPosition.y-1) return true;
    return false;
}

bool QueenValidity(Position oldPosition, Position newPosition,int chessBoard[8][8]){
    return (RookValidity(oldPosition, newPosition,chessBoard) || BishopValidity(oldPosition, newPosition,chessBoard));
}

bool KingValidity(Position oldPosition, Position newPosition, int chessBoard[8][8]){
    if(chessBoard[oldPosition.x][oldPosition.y] == 0) return false;
    if(abs(newPosition.x-oldPosition.x) <= 1 && abs(newPosition.y-oldPosition.y) <= 1) return true;
    return false;
}