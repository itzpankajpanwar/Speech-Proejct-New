#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include "validate.h"
#include "HMM.h"
using namespace sf;

Sprite pieces[32];
int piece_size = 56;
int n_pieces = 32;
bool turn = false;
bool gameOver = false;
bool hasMoved = true;
bool check = false;
Text winnerText;
Font font;
Position blackKingPosition = {0,4};
Position whiteKingPosition = {7,4};

int chessBoard[8][8] = {
    {-1,-2,-3,-4,-5,-3,-2,-1},
    {-6,-6,-6,-6,-6,-6,-6,-6},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {6, 6, 6, 6, 6, 6, 6, 6},
    {1, 2, 3, 4, 5, 3, 2, 1}};

Position toPosition(char a, char b){
    Position position;
    position.x = 8 - (int)(b-'0');
    position.y = (int)(a-'a');
    return position; 
}

std::string toMove(Position curPosition, Position kingPosition){
    std::string move = "";
    move += 'a' + curPosition.y;
    move += '0' + 8 - curPosition.x;
    move += 'a' + kingPosition.y;
    move += '0' + 8 - kingPosition.x;
    return move;
}

void customizeMoveText(Text& text, std::string move){
    text.setFont(font);
    text.setString(move);
    text.setCharacterSize(15);
    text.setPosition(586,75);
    text.setStyle(Text::Style::Bold);
}

void customizeTurnText(Text& text, std::string t){
    text.setFont(font);
    text.setString(t);
    text.setCharacterSize(15);
    text.setFillColor(Color(66,165,245));
    text.setPosition(535,97);
}

void customizeCheckText(Text& text){
    text.setFont(font);
    text.setString("check!");
    text.setCharacterSize(15);
    text.setFillColor(Color(255,214,0));
    text.setPosition(560,56);
}

void customizeButtonText(Text& text){
    text.setFont(font);
    text.setString("Speak");
    text.setCharacterSize(11);
    text.setFillColor(Color(224,224,224));
    text.setPosition(570,124);
}

void customizeWinnerText(std::string winner){
    Font* font = new Font();
    font->loadFromFile("res/fonts/GreatVibes-Regular.otf");
    Color fontColor = winner[0]=='W' ? Color::White : Color::Black;
    Color outlineColor = winner[0]=='W' ? Color::Black : Color::White;
    winnerText.setFont(*font);
    winnerText.setString(winner);
    winnerText.setCharacterSize(50);
    winnerText.setFillColor(fontColor);
    winnerText.setPosition(140,210);
    winnerText.setStyle(Text::Style::Bold);
    winnerText.setOutlineColor(outlineColor);
    winnerText.setOutlineThickness(2);
}

bool isMouseOnButton(RenderWindow& window, RectangleShape button){
    float mouseX = Mouse::getPosition(window).x;
    float mouseY = Mouse::getPosition(window).y;
    float buttonX = button.getPosition().x;
    float buttonY = button.getPosition().y;
    float buttonH = button.getLocalBounds().height;
    float buttonW = button.getLocalBounds().width;

    if(mouseX < buttonX || mouseX > buttonX + buttonW || mouseY < buttonY || mouseY > buttonY + buttonH) return false;
    return true; 
}

bool isValidMove(std::string move){
    if(move.length() != 4) return false;
    Position oldPosition = toPosition(move[0], move[1]);
    Position newPosition = toPosition(move[2], move[3]);

    if(oldPosition.x<0 || oldPosition.y<0 || newPosition.x<0 || newPosition.y<0) return false;
    else if(oldPosition.x>7 || oldPosition.y>7 || newPosition.x>7 || newPosition.y>7) return false;
    else if(chessBoard[oldPosition.x][oldPosition.y] == 0) return false;
    else if(turn && (chessBoard[oldPosition.x][oldPosition.y]>0 || chessBoard[newPosition.x][newPosition.y]<0)) return false;
    else if(!turn && (chessBoard[oldPosition.x][oldPosition.y]<0 || chessBoard[newPosition.x][newPosition.y]>0)) return false;
    
    int curPiece = chessBoard[oldPosition.x][oldPosition.y];
    if(curPiece == 6) return whitePawnValidity(oldPosition, newPosition, chessBoard);
    else if(curPiece == -6) return blackPawnValidity(oldPosition, newPosition, chessBoard);
    else{
        switch(abs(curPiece)){
            case 1: return RookValidity(oldPosition, newPosition, chessBoard); break;
            case 2: return KnightValidity(oldPosition, newPosition, chessBoard); break;
            case 3: return BishopValidity(oldPosition, newPosition, chessBoard); break;
            case 4: return QueenValidity(oldPosition, newPosition, chessBoard); break;
            case 5: return KingValidity(oldPosition, newPosition, chessBoard); break;
        }
    }

    return false;
}

void inCheck(){
    Position kingPosition = turn ?  whiteKingPosition : blackKingPosition;
    for(int r=0; r<8; ++r){
        for(int c=0; c<8; ++c){
            if(turn && chessBoard[r][c] >= 0) continue;
            if(!turn && chessBoard[r][c] <= 0) continue;
            Position curPosition = {r,c};
            std::string move = toMove(curPosition, kingPosition);
            check = isValidMove(move);
            if(check) return;
        }
    }
}

void loadPosition(){
    int i = 0;
    for(int r=0; r<8; ++r){
        for(int c=0; c<8; ++c){
            int x = chessBoard[r][c];
            if(x == 0) continue;
            int y = x<0 ? 0 : 1;
            x = abs(x) - 1;
            pieces[i].setTextureRect(IntRect(piece_size*x, piece_size*y, piece_size, piece_size));
            pieces[i].setPosition(piece_size*c+28, piece_size*r+26);
            if(++i == n_pieces) return;
        }
    }
}

void makeMove(std::string move){
    Position oldPosition = toPosition(move[0], move[1]);
    Position newPosition = toPosition(move[2], move[3]);

    if(chessBoard[newPosition.x][newPosition.y]) n_pieces--;
    if(chessBoard[newPosition.x][newPosition.y] == 5 || chessBoard[newPosition.x][newPosition.y] == -5){
        std::string winner = turn ? "Black wins!" : "White wins!";
        customizeWinnerText(winner);
        gameOver = true;
    }

    chessBoard[newPosition.x][newPosition.y] = chessBoard[oldPosition.x][oldPosition.y];
    chessBoard[oldPosition.x][oldPosition.y] = 0;
    
    if(chessBoard[newPosition.x][newPosition.y] == 5) whiteKingPosition = {newPosition.x, newPosition.y};
    else if(chessBoard[newPosition.x][newPosition.y] == -5) blackKingPosition = {newPosition.x, newPosition.y};

    loadPosition();
    inCheck();
}

int main(){
    RenderWindow window(VideoMode(662, 504), "Chess", Style::Default);
    window.setKeyRepeatEnabled(false);
    Image icon;
    icon.loadFromFile("res/images/icon.png");
    window.setIcon(512,512, icon.getPixelsPtr());

    SoundBuffer checkBuffer;
    checkBuffer.loadFromFile("res/sound/check.wav");
    Sound checkSound;
    checkSound.setBuffer(checkBuffer);

    SoundBuffer winBuffer;
    winBuffer.loadFromFile("res/sound/win.wav");
    Sound winSound;
    winSound.setBuffer(winBuffer);

    RectangleShape button;
    button.setPosition(556, 120);
    button.setSize({60, 25});
    button.setFillColor(Color(51, 171, 101));
    button.setOutlineColor(Color::Black);
    button.setOutlineThickness(1);

    Texture pieceTexture, boardTexture;
    pieceTexture.loadFromFile("res/images/pieces.png");
    boardTexture.loadFromFile("res/images/chessboard.png");
    for(int i=0;i<32;i++) pieces[i].setTexture(pieceTexture);
    Sprite board(boardTexture);
    loadPosition();

    Text moveText, turnText, checkText, buttonText;
    font.loadFromFile("res/fonts/RobotoSlab-Regular.ttf");
    customizeMoveText(moveText, "");
    customizeTurnText(turnText, "White's Turn");
    customizeCheckText(checkText);
    customizeButtonText(buttonText);

    int cnt = 0;
    std::string move = "";
    while(window.isOpen()){
        Event e;
        bool resizeWindow = false;
        while(window.pollEvent(e)){
            switch(e.type){
                case Event::Closed : window.close(); break;
                case Event::Resized : resizeWindow = true; break;
                case Event::MouseButtonPressed :
                    if(!isMouseOnButton(window, button)) break; 
                    if(gameOver) break;

                    move += recognizeWord();
                    if(++cnt < 4) break;
                    moveText.setString(move);
                    if(!isValidMove(move)) moveText.setFillColor(Color(255,0,51));
                    else{
                        makeMove(move);
                        moveText.setFillColor(Color(75,181,67));
                        if(turn) turnText.setString("White's Turn");
                        else turnText.setString("Blacks's Turn");
                        turn = !turn;
                    }
                    
                    hasMoved = true;
                    cnt = 0;
                    move = "";
                    std::cout << "Button Clicked" << std::endl;
                    break;
                default: break;
            }
        }
        
        if(hasMoved || resizeWindow){
            window.clear();
            window.draw(board);
            for(int i=0; i<n_pieces; ++i) window.draw(pieces[i]);
            if(gameOver) window.draw(winnerText);
            if(gameOver && !resizeWindow) winSound.play();
            if(check) window.draw(checkText);
            if(check && !resizeWindow) checkSound.play();
            window.draw(moveText);
            window.draw(turnText);
            window.draw(button);
            window.draw(buttonText);
            window.display();
            hasMoved = false;
            resizeWindow = false;
        }
    }

    return 0;
}