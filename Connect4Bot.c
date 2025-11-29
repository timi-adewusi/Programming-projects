#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define ROWS 6
#define COLS 7
#define PLAYER 1
#define AI 2
#define MAX_DEPTH 10
#define CELL_SIZE 80
#define PADDING 10
#define MAX_GAMES 100

int board[ROWS][COLS];

// Adaptive heuristic weights
float weight_window = 1.0f;
float weight_center = 0.6f;
float weight_parity = 0.3f;

// Game history
struct GameRecord {
    int moves[ROWS*COLS];
    int moveCount;
    int winner; // 0=draw, 1=player, 2=AI
} gameHistory[MAX_GAMES];
int gameCount = 0;
struct GameRecord currentGame;

// Function prototypes
void initBoard();
void drawBoard();
int isValidMove(int col);
int makeMove(int col, int player);
void undoMove(int col);
int checkWin(int player);
int checkDraw();
int evaluateWindow(int window[4], int player);
int scorePosition(int player);
int minimax(int depth, int alpha, int beta, int maximizingPlayer);
int getBestMove();
int findImmediateWinOrBlock(int player);
int rowParityBonus(int col, int player);
void updateHeuristicWeights();

void initBoard() {
    for(int r = 0; r < ROWS; r++)
        for(int c = 0; c < COLS; c++)
            board[r][c] = 0;
    currentGame.moveCount = 0;
}

void drawBoard() {
    ClearBackground(RAYWHITE);
    for(int r = 0; r < ROWS; r++) {
        for(int c = 0; c < COLS; c++) {
            Color color = LIGHTGRAY;
            if(board[r][c] == PLAYER) color = RED;
            else if(board[r][c] == AI) color = YELLOW;
            DrawRectangle(c*CELL_SIZE + PADDING, r*CELL_SIZE + PADDING,
                          CELL_SIZE - 2*PADDING, CELL_SIZE - 2*PADDING, color);
            DrawRectangleLines(c*CELL_SIZE + PADDING, r*CELL_SIZE + PADDING,
                               CELL_SIZE - 2*PADDING, CELL_SIZE - 2*PADDING, BLACK);
        }
    }
}

int isValidMove(int col) { return board[0][col] == 0; }

int makeMove(int col, int player) {
    for(int r = ROWS - 1; r >= 0; r--) {
        if(board[r][col] == 0) {
            board[r][col] = player;
            currentGame.moves[currentGame.moveCount++] = col; // Track move
            return r;
        }
    }
    return -1;
}

void undoMove(int col) {
    for(int r = 0; r < ROWS; r++) {
        if(board[r][col] != 0) {
            board[r][col] = 0;
            currentGame.moveCount--; // Undo move in current game
            break;
        }
    }
}

int checkWin(int player) {
    for(int r = 0; r < ROWS; r++)
        for(int c = 0; c < COLS - 3; c++)
            if(board[r][c]==player && board[r][c+1]==player &&
               board[r][c+2]==player && board[r][c+3]==player) return 1;
    for(int r = 0; r < ROWS - 3; r++)
        for(int c = 0; c < COLS; c++)
            if(board[r][c]==player && board[r+1][c]==player &&
               board[r+2][c]==player && board[r+3][c]==player) return 1;
    for(int r = 3; r < ROWS; r++)
        for(int c = 0; c < COLS - 3; c++)
            if(board[r][c]==player && board[r-1][c+1]==player &&
               board[r-2][c+2]==player && board[r-3][c+3]==player) return 1;
    for(int r = 0; r < ROWS - 3; r++)
        for(int c = 0; c < COLS - 3; c++)
            if(board[r][c]==player && board[r+1][c+1]==player &&
               board[r+2][c+2]==player && board[r+3][c+3]==player) return 1;
    return 0;
}

int checkDraw() {
    for(int c = 0; c < COLS; c++)
        if(board[0][c] == 0) return 0;
    return 1;
}

int evaluateWindow(int window[4], int player) {
    int score = 0;
    int opp = (player == PLAYER) ? AI : PLAYER;
    int countPlayer = 0, countEmpty = 0, countOpp = 0;

    for(int i = 0; i < 4; i++) {
        if(window[i] == player) countPlayer++;
        else if(window[i] == opp) countOpp++;
        else countEmpty++;
    }

    if(countPlayer == 4) score += 1000;
    else if(countPlayer == 3 && countEmpty == 1) score += 50;
    else if(countPlayer == 2 && countEmpty == 2) score += 10;

    if(countOpp == 3 && countEmpty == 1) score -= 90;
    if(countOpp == 2 && countEmpty == 2) score -= 5;

    return score;
}

int rowParityBonus(int col, int player) {
    int r;
    for(r = ROWS - 1; r >= 0; r--) {
        if(board[r][col] == 0) break;
    }
    if(r < 0) return 0;
    if(player == AI) return (r % 2 == 0) ? 10 : 5;
    else return (r % 2 == 1) ? -10 : -5;
}

int scorePosition(int player) {
    int score = 0;
    // Center column
    for(int r = 0; r < ROWS; r++)
        if(board[r][COLS/2] == player) score += (int)(weight_center * 6);

    // Horizontal
    for(int r = 0; r < ROWS; r++)
        for(int c = 0; c < COLS - 3; c++) {
            int window[4]; for(int i = 0; i < 4; i++) window[i] = board[r][c+i];
            score += (int)(weight_window * evaluateWindow(window, player));
        }

    // Vertical
    for(int r = 0; r < ROWS - 3; r++)
        for(int c = 0; c < COLS; c++) {
            int window[4]; for(int i = 0; i < 4; i++) window[i] = board[r+i][c];
            score += (int)(weight_window * evaluateWindow(window, player));
        }

    // Positive diagonal
    for(int r = 3; r < ROWS; r++)
        for(int c = 0; c < COLS - 3; c++) {
            int window[4]; for(int i = 0; i < 4; i++) window[i] = board[r-i][c+i];
            score += (int)(weight_window * evaluateWindow(window, player));
        }

    // Negative diagonal
    for(int r = 0; r < ROWS - 3; r++)
        for(int c = 0; c < COLS - 3; c++) {
            int window[4]; for(int i = 0; i < 4; i++) window[i] = board[r+i][c+i];
            score += (int)(weight_window * evaluateWindow(window, player));
        }

    // Row parity
    for(int c = 0; c < COLS; c++)
        if(isValidMove(c)) score += (int)(weight_parity * rowParityBonus(c, player));

    return score;
}

int minimax(int depth, int alpha, int beta, int maximizingPlayer) {
    if(checkWin(AI)) return 1000000;
    if(checkWin(PLAYER)) return -1000000;
    if(depth == 0) return scorePosition(AI);

    int order[COLS] = {3,2,4,1,5,0,6};
    if(maximizingPlayer) {
        int maxEval = INT_MIN;
        for(int i = 0; i < COLS; i++) {
            int c = order[i];
            if(isValidMove(c)) {
                makeMove(c, AI);
                int eval = minimax(depth - 1, alpha, beta, 0);
                undoMove(c);
                if(eval > maxEval) maxEval = eval;
                if(eval > alpha) alpha = eval;
                if(beta <= alpha) break;
            }
        }
        return maxEval;
    } else {
        int minEval = INT_MAX;
        for(int i = 0; i < COLS; i++) {
            int c = order[i];
            if(isValidMove(c)) {
                makeMove(c, PLAYER);
                int eval = minimax(depth - 1, alpha, beta, 1);
                undoMove(c);
                if(eval < minEval) minEval = eval;
                if(eval < beta) beta = eval;
                if(beta <= alpha) break;
            }
        }
        return minEval;
    }
}

int findImmediateWinOrBlock(int player) {
    for(int c = 0; c < COLS; c++) {
        if(!isValidMove(c)) continue;
        makeMove(c, player);
        if(checkWin(player)) { undoMove(c); return c; }
        undoMove(c);
    }
    return -1;
}

int getBestMove() {
    int winCol = findImmediateWinOrBlock(AI);
    if(winCol != -1) return winCol;
    int blockCol = findImmediateWinOrBlock(PLAYER);
    if(blockCol != -1) return blockCol;

    int bestScore = INT_MIN, bestCol = 3;
    int order[COLS] = {3,2,4,1,5,0,6};
    for(int i = 0; i < COLS; i++) {
        int c = order[i];
        if(isValidMove(c)) {
            makeMove(c, AI);
            int score = minimax(MAX_DEPTH - 1, INT_MIN, INT_MAX, 0);
            undoMove(c);
            if(score > bestScore) { bestScore = score; bestCol = c; }
        }
    }
    return bestCol;
}

// Adaptive heuristic weight update
void updateHeuristicWeights() {
    if(currentGame.winner == 0) return; // draw, skip
    // Simple adaptive rule: adjust weights slightly
    float adjust = 0.05f; 
    if(currentGame.winner == AI) {
        weight_window += adjust; weight_center += adjust; weight_parity += adjust;
    } else if(currentGame.winner == PLAYER) {
        weight_window -= adjust; weight_center -= adjust; weight_parity -= adjust;
    }
    // Clamp weights to reasonable range
    if(weight_window < 0.1f) weight_window = 0.1f;
    if(weight_center < 0.1f) weight_center = 0.1f;
    if(weight_parity < 0.1f) weight_parity = 0.1f;
    if(weight_window > 5.0f) weight_window = 5.0f;
    if(weight_center > 5.0f) weight_center = 5.0f;
    if(weight_parity > 5.0f) weight_parity = 5.0f;
    // Store game in history
    if(gameCount < MAX_GAMES) gameHistory[gameCount++] = currentGame;
}

int main() {
    InitWindow(COLS*CELL_SIZE, ROWS*CELL_SIZE, "Connect 4 Adaptive HROG AI");
    SetTargetFPS(60);

    while(!WindowShouldClose()) {
        initBoard();
        int turn = PLAYER, winner = 0;
        currentGame.moveCount = 0;

        while(winner == 0) {
            drawBoard();
            if(turn == PLAYER) {
                if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    int col = GetMouseX() / CELL_SIZE;
                    if(isValidMove(col)) {
                        makeMove(col, PLAYER);
                        if(checkWin(PLAYER)) winner = PLAYER;
                        else if(checkDraw()) winner = 3;
                        turn = AI;
                    }
                }
            } else {
                int aiMove = getBestMove();
                makeMove(aiMove, AI);
                if(checkWin(AI)) winner = AI;
                else if(checkDraw()) winner = 3;
                turn = PLAYER;
            }

            if(winner != 0) {
                currentGame.winner = winner;
                updateHeuristicWeights();
                drawBoard();
                if(winner == PLAYER) DrawText("You Win!", 10,10,40,GREEN);
                else if(winner == AI) DrawText("AI Wins!", 10,10,40,RED);
                else DrawText("Draw!", 10,10,40,GRAY);
                EndDrawing();
                WaitTime(3.0f);
                break;
            }
            EndDrawing();
        }
    }
    CloseWindow();
    return 0;
}
