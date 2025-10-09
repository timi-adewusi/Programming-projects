//
//  main.c
//  LittleProject
//
//  Created by Timi Adewusi on 26/03/2023.
//
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
#include <unistd.h>
#include <limits.h>

#define REDPIECE 1
#define YELLOWPIECE 2

#define ROWS 6
#define COLS 7

const int screenWidth = 490;
const int screenHeight = 420;

const int cellWidth = 70;
const int cellHeight = 70;

void drawCell(int cell[COLS][ROWS]);
bool indexIsValid(int, int);
void cellOccupy(int, int);
bool checkWin(int cell[COLS][ROWS], int piece);
int checkTwo(int cell[COLS][ROWS], int piece);
int checkThree(int cell[COLS][ROWS], int piece);
int checkFour(int cell[COLS][ROWS], int piece);
int calculateScore(int cell[COLS][ROWS], int piece);
void computerMove(int board[COLS][ROWS], int computerPiece);
int miniMax(int board[COLS][ROWS], int depth, int alpha, int beta, bool isMaximising);
bool isTerminalNode(int board[COLS][ROWS]);
int max(int x, int y);
int min(int x, int y);
void clearBoard(int board[COLS][ROWS]);

int main(void)
{
    bool isPlayerTurn = false;
    int pieceCount = 0;
    
    int grid[COLS][ROWS] = {0};
    
    InitWindow(screenWidth, screenHeight, "Connect 4");
    
    SetTargetFPS(60);
    
    while(!WindowShouldClose())
    {
        if(!checkWin(grid, REDPIECE) && !checkWin(grid, YELLOWPIECE) && pieceCount != 42)
        {
            if(isPlayerTurn)
            {
                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                {
                    Vector2 mPos = GetMousePosition();
                    int indexI = mPos.x / cellWidth;
                    
                    for(int j = 5; j >= 0; j--)
                    {
                        if(grid[indexI][j] == 0)
                        {
                            grid[indexI][j] = REDPIECE;
                            pieceCount++;
                            isPlayerTurn = false;
                            break;
                        }
                    }
                }
            }
            else
            {
                computerMove(grid, YELLOWPIECE);
                pieceCount++;
                isPlayerTurn = true;
            }
        }
        
        BeginDrawing();
        ClearBackground(BLACK);
        
        drawCell(grid);
        
        if(checkWin(grid, REDPIECE))
        {
            DrawText("RED WINS", 10, 10, 40, GREEN);
        }
        else if(checkWin(grid,YELLOWPIECE))
        {
            DrawText("YELLOW WINS", 10, 10, 40, GREEN);
        }
        else if(pieceCount == 42)
        {
            DrawText("DRAW", 10, 10, 40, GREEN);
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
void drawCell(int cell[COLS][ROWS])
{
    for(int i = 0; i < COLS; i++)
    {
        for(int j = 0; j < ROWS; j++)
        {
            if(cell[i][j] == 1)
            {
                DrawCircle((i * cellWidth)+ 35, ((j * cellHeight) + 35), 35, RED);
            }
            else if(cell[i][j] == 2)
            {
                DrawCircle((i * cellWidth)+ 35, ((j * cellHeight) + 35), 35, YELLOW);
            }
            DrawRectangleLines(i * cellWidth, j * cellHeight, cellWidth, cellHeight, WHITE);
        }
    }
}
bool indexIsValid(int i, int j)
{
    return i >= 0 && i < COLS && j >= 0 && j < ROWS;
}
bool checkWin(int cell[COLS][ROWS], int piece)
{
    //vertical
    for(int i = 0; i < COLS; i++)
    {
        for(int j = 0; j < 3; j++)
        {
            if(cell[i][j] == piece && cell[i][j+1] == piece && cell[i][j+2] == piece && cell[i][j+3] == piece)
            {
                return true;
            }
        }
    }
    
    //horizontal
    for(int i = 0; i < 4; i++)
    {
        for(int j = 0; j < ROWS; j++)
        {
            if(cell[i][j] == piece && cell[i+1][j] == piece && cell[i+2][j] == piece && cell[i+3][j] == piece)
            {
                return true;
            }
        }
    }
    
    //diagonal up
    for(int i = 0; i < 4; i++)
    {
        for(int j = 3; j < ROWS; j++)
        {
            if(cell[i][j] == piece && cell[i+1][j-1] == piece && cell[i+2][j-2] == piece && cell[i+3][j-3] == piece)
            {
                return true;
            }
        }
    }
    
    //diagonal down
    for(int i = 0; i < 4; i++)
    {
        for(int j = 0; j < 3; j++)
        {
            if(cell[i][j] == piece && cell[i+1][j+1] == piece && cell[i+2][j+2] == piece && cell[i+3][j+3] == piece)
            {
                return true;
            }
        }
    }
    return false;
}

int checkFour(int cell[COLS][ROWS], int piece)
{
    int fourCount = 0;
    //vertical
    for(int i = 0; i < COLS; i++)
    {
        for(int j = 0; j < 3; j++)
        {
            if(cell[i][j] == piece && cell[i][j+1] == piece && cell[i][j+2] == piece && cell[i][j+3] == piece)
            {
                fourCount++;
            }
        }
    }
    
    //horizontal
    for(int i = 0; i < 4; i++)
    {
        for(int j = 0; j < ROWS; j++)
        {
            if(cell[i][j] == piece && cell[i+1][j] == piece && cell[i+2][j] == piece && cell[i+3][j] == piece)
            {
                fourCount++;
            }
        }
    }
    
    //diagonal up
    for(int i = 0; i < 4; i++)
    {
        for(int j = 3; j < ROWS; j++)
        {
            if(cell[i][j] == piece && cell[i+1][j-1] == piece && cell[i+2][j-2] == piece && cell[i+3][j-3] == piece)
            {
                fourCount++;
            }
        }
    }
    
    //diagonal down
    for(int i = 0; i < 4; i++)
    {
        for(int j = 0; j < 3; j++)
        {
            if(cell[i][j] == piece && cell[i+1][j+1] == piece && cell[i+2][j+2] == piece && cell[i+3][j+3] == piece)
            {
                fourCount++;
            }
        }
    }
    return fourCount;
}

int checkThree(int cell[COLS][ROWS], int piece)
{
    int threeCount = 0;
    //vertical
    for(int i = 0; i < COLS; i++)
    {
        for(int j = 0; j < 3; j++)
        {
            if(cell[i][j] == 0 && cell[i][j+1] == piece && cell[i][j+2] == piece && cell[i][j+3] == piece)
            {
                threeCount++;
            }
        }
    }
    
    //horizontal
    for(int i = 0; i < 4; i++)
    {
        for(int j = 0; j < ROWS; j++)
        {
            if(cell[i][j] == 0 && cell[i+1][j] == piece && cell[i+2][j] == piece && cell[i+3][j] == piece)
            {
                threeCount++;
            }
            else if(cell[i][j] == piece && cell[i+1][j] == piece && cell[i+2][j] == piece && cell[i+3][j] == 0)
            {
                threeCount++;
            }
            else if(cell[i][j] == piece && cell[i+1][j] == 0 && cell[i+2][j] == piece && cell[i+3][j] == piece)
            {
                threeCount++;
            }
            else if(cell[i][j] == piece && cell[i+1][j] == piece && cell[i+2][j] == 0 && cell[i+3][j] == piece)
            {
                threeCount++;
            }
        }
    }
    
    //diagonal up
    for(int i = 0; i < 4; i++)
    {
        for(int j = 3; j < ROWS; j++)
        {
            if(cell[i][j] == 0 && cell[i+1][j-1] == piece && cell[i+2][j-2] == piece && cell[i+3][j-3] == piece)
            {
                threeCount++;
            }
            else if(cell[i][j] == piece && cell[i+1][j-1] == piece && cell[i+2][j-2] == piece && cell[i+3][j-3] == 0)
            {
                threeCount++;
            }
            else if(cell[i][j] == piece && cell[i+1][j-1] == 0 && cell[i+2][j-2] == piece && cell[i+3][j-3] == piece)
            {
                threeCount++;
            }
            else if(cell[i][j] == piece && cell[i+1][j-1] == piece && cell[i+2][j-2] == 0 && cell[i+3][j-3] == piece)
            {
                threeCount++;
            }
        }
    }
    
    //diagonal down
    for(int i = 0; i < 4; i++)
    {
        for(int j = 0; j < 3; j++)
        {
            if(cell[i][j] == 0 && cell[i+1][j+1] == piece && cell[i+2][j+2] == piece && cell[i+3][j+3] == piece)
            {
                threeCount++;
            }
            else if(cell[i][j] == piece && cell[i+1][j+1] == piece && cell[i+2][j+2] == piece && cell[i+3][j+3] == 0)
            {
                threeCount++;
            }
            else if(cell[i][j] == piece && cell[i+1][j+1] == 0 && cell[i+2][j+2] == piece && cell[i+3][j+3] == piece)
            {
                threeCount++;
            }
            else if(cell[i][j] == piece && cell[i+1][j+1] == piece && cell[i+2][j+2] == 0 && cell[i+3][j+3] == piece)
            {
                threeCount++;
            }
        }
    }
    return threeCount;
}
int checkTwo(int cell[COLS][ROWS], int piece)
{
    int twoCount = 0;
    //vertical
    for(int i = 0; i < COLS; i++)
    {
        for(int j = 0; j < 4; j++)
        {
            if(cell[i][j] == 0 && cell[i][j+1] == piece && cell[i][j+2] == piece)
            {
                twoCount++;
            }
        }
    }
    
    //horizontal
    for(int i = 0; i < 4; i++)
    {
        for(int j = 0; j < ROWS; j++)
        {
            if(cell[i][j] == piece && cell[i+1][j] == piece && cell[i+2][j] == 0 && cell[i+3][j] == 0)
            {
                twoCount++;
            }
            else if(cell[i][j] == 0 && cell[i+1][j] == piece && cell[i+2][j] == piece && cell[i+3][j] == 0)
            {
                twoCount++;
            }
            else if(cell[i][j] == 0 && cell[i+1][j] == 0 && cell[i+2][j] == piece && cell[i+3][j] == piece)
            {
                twoCount++;
            }
        }
    }
    
    //diagonal up
    for(int i = 0; i < 4; i++)
    {
        for(int j = 3; j < ROWS; j++)
        {
            if(cell[i][j] == piece && cell[i+1][j-1] == piece && cell[i+2][j-2] == 0 && cell[i+3][j-3] == 0)
            {
                twoCount++;
            }
            else if(cell[i][j] == 0 && cell[i+1][j-1] == piece && cell[i+2][j-2] == piece && cell[i+3][j-3] == 0)
            {
                twoCount++;
            }
            else if(cell[i][j] == 0 && cell[i+1][j-1] == 0 && cell[i+2][j-2] == piece && cell[i+3][j-3] == piece)
            {
                twoCount++;
            }
        }
    }
    
    //diagonal down
    for(int i = 0; i < 4; i++)
    {
        for(int j = 0; j < 3; j++)
        {
            if(cell[i][j] == piece && cell[i+1][j+1] == piece && cell[i+2][j+2] == 0 && cell[i+3][j+3] == 0)
            {
                twoCount++;
            }
            else if(cell[i][j] == 0 && cell[i+1][j+1] == piece && cell[i+2][j+2] == piece && cell[i+3][j+3] == 0)
            {
                twoCount++;
            }
            else if(cell[i][j] == 0 && cell[i+1][j+1] == 0 && cell[i+2][j+2] == piece && cell[i+3][j+3] == piece)
            {
                twoCount++;
            }
        }
    }
    return twoCount;
}

int checkOddOrEven(int cell[COLS][ROWS], int piece)
{
    int threeCount = 0;
    
    if (piece == 2)
    {
        //vertical
        for(int i = 0; i < COLS; i++)
        {
            for(int j = 0; j < 3; j++)
            {
                if(cell[i][j] == 0 && cell[i][j+1] == piece && cell[i][j+2] == piece && cell[i][j+3] == piece)
                {
                    threeCount += 2;
                    if(j == 1 || j == 3 || j == 5)
                    {
                        threeCount += 40;
                    }
                }
            }
        }
        
        //horizontal
        for(int i = 0; i < 4; i++)
        {
            for(int j = 0; j < ROWS; j++)
            {
                if(cell[i][j] == 0 && cell[i+1][j] == piece && cell[i+2][j] == piece && cell[i+3][j] == piece)
                {
                    threeCount++;
                    if(j == 1 || j == 3 || j == 5)
                    {
                        threeCount += 2;
                        if(j == 5){
                            threeCount += 50;
                        }
                    }
                }
                else if(cell[i][j] == piece && cell[i+1][j] == piece && cell[i+2][j] == piece && cell[i+3][j] == 0)
                {
                    threeCount++;
                    if(j == 1 || j == 3 || j == 5)
                    {
                        threeCount += 2;
                        if(j == 5){
                            threeCount += 50;
                        }
                    }
                }
                else if(cell[i][j] == piece && cell[i+1][j] == 0 && cell[i+2][j] == piece && cell[i+3][j] == piece)
                {
                    threeCount++;
                    if(j == 1 || j == 3 || j == 5)
                    {
                        threeCount += 2;
                        if(j == 5){
                            threeCount += 50;
                        }
                    }
                }
                else if(cell[i][j] == piece && cell[i+1][j] == piece && cell[i+2][j] == 0 && cell[i+3][j] == piece)
                {
                    threeCount++;
                    if(j == 1 || j == 3 || j == 5)
                    {
                        threeCount += 2;
                        if(j == 5){
                            threeCount += 50;
                        }
                    }
                }
            }
        }
        
        //diagonal up
        for(int i = 0; i < 4; i++)
        {
            for(int j = 3; j < ROWS; j++)
            {
                if(cell[i][j] == 0 && cell[i+1][j-1] == piece && cell[i+2][j-2] == piece && cell[i+3][j-3] == piece)
                {
                    threeCount++;
                    if(j == 1 || j == 3 || j == 5)
                    {
                        threeCount += 2;
                        if(j+1 != 0){
                            threeCount += 50;
                        }
                    }
                }
                else if(cell[i][j] == piece && cell[i+1][j-1] == piece && cell[i+2][j-2] == piece && cell[i+3][j-3] == 0)
                {
                    threeCount++;
                    if(j-3 == 1 || j-3 == 3 || j-3 == 5)
                    {
                        threeCount += 2;
                        if(j-2 != 0){
                            threeCount += 50;
                        }
                    }
                }
                else if(cell[i][j] == piece && cell[i+1][j-1] == 0 && cell[i+2][j-2] == piece && cell[i+3][j-3] == piece)
                {
                    threeCount++;
                    if(j-1 == 1 || j-1 == 3 || j-1 == 5)
                    {
                        threeCount += 2;
                        if(j != 0){
                            threeCount += 50;
                        }
                    }
                }
                else if(cell[i][j] == piece && cell[i+1][j-1] == piece && cell[i+2][j-2] == 0 && cell[i+3][j-3] == piece)
                {
                    threeCount++;
                    if(j-2 == 1 || j-2 == 3 || j-2 == 5)
                    {
                        threeCount += 2;
                        if(j-1 != 0){
                            threeCount += 50;
                        }
                    }
                }
            }
        }
        
        //diagonal down
        for(int i = 0; i < 4; i++)
        {
            for(int j = 0; j < 3; j++)
            {
                if(cell[i][j] == 0 && cell[i+1][j+1] == piece && cell[i+2][j+2] == piece && cell[i+3][j+3] == piece)
                {
                    threeCount++;
                    if(j == 1 || j == 3 || j == 5)
                    {
                        threeCount += 2;
                        if(j+1 != 0){
                            threeCount += 50;
                        }
                    }
                }
                else if(cell[i][j] == piece && cell[i+1][j+1] == piece && cell[i+2][j+2] == piece && cell[i+3][j+3] == 0)
                {
                    threeCount++;
                    if(j+3 == 1 || j+3 == 3 || j+3 == 5)
                    {
                        threeCount += 2;
                        if(j+4 != 0){
                            threeCount += 50;
                        }
                    }
                }
                else if(cell[i][j] == piece && cell[i+1][j+1] == 0 && cell[i+2][j+2] == piece && cell[i+3][j+3] == piece)
                {
                    threeCount++;
                    if(j+1 == 1 || j+1 == 3 || j+1 == 5)
                    {
                        threeCount += 2;
                        if(j+2 != 0){
                            threeCount += 50;
                        }
                    }
                }
                else if(cell[i][j] == piece && cell[i+1][j+1] == piece && cell[i+2][j+2] == 0 && cell[i+3][j+3] == piece)
                {
                    threeCount++;
                    if(j+2 == 1 || j+2 == 3 || j+2 == 5)
                    {
                        threeCount += 2;
                        if(j+3 != 0){
                            threeCount += 50;
                        }
                    }
                }
            }
        }
    }
    else
    {
        for(int i = 0; i < COLS; i++)
        {
            for(int j = 0; j < 3; j++)
            {
                if(cell[i][j] == 0 && cell[i][j+1] == piece && cell[i][j+2] == piece && cell[i][j+3] == piece)
                {
                    threeCount +=2;
                    if(j == 0 || j == 2 || j == 4)
                    {
                        threeCount+= 40;
                    }
                }
            }
        }
        
        //horizontal
        for(int i = 0; i < 4; i++)
        {
            for(int j = 0; j < ROWS; j++)
            {
                if(cell[i][j] == 0 && cell[i+1][j] == piece && cell[i+2][j] == piece && cell[i+3][j] == piece)
                {
                    threeCount++;
                    if(j == 0 || j == 2 || j == 4)
                    {
                        threeCount += 2;
                        if(j+1 != 0){
                            threeCount += 50;
                        }
                    }
                }
                else if(cell[i][j] == piece && cell[i+1][j] == piece && cell[i+2][j] == piece && cell[i+3][j] == 0)
                {
                    threeCount++;
                    if(j == 0 || j == 2 || j == 4)
                    {
                        threeCount += 2;
                        if(j+1 != 0){
                            threeCount += 50;
                        }
                    }
                }
                else if(cell[i][j] == piece && cell[i+1][j] == 0 && cell[i+2][j] == piece && cell[i+3][j] == piece)
                {
                    threeCount++;
                    if(j == 0 || j == 2 || j == 4)
                    {
                        threeCount += 2;
                        if(j+1 != 0){
                            threeCount += 50;
                        }
                    }
                }
                else if(cell[i][j] == piece && cell[i+1][j] == piece && cell[i+2][j] == 0 && cell[i+3][j] == piece)
                {
                    threeCount++;
                    if(j == 0 || j == 2 || j == 4)
                    {
                        threeCount += 2;
                        if(j+1 != 0){
                            threeCount += 50;
                        }
                    }
                }
            }
        }
        
        //diagonal up
        for(int i = 0; i < 4; i++)
        {
            for(int j = 3; j < ROWS; j++)
            {
                if(cell[i][j] == 0 && cell[i+1][j-1] == piece && cell[i+2][j-2] == piece && cell[i+3][j-3] == piece)
                {
                    threeCount++;
                    if(j == 0 || j == 2 || j == 4)
                    {
                        threeCount += 2;
                        if(j+1 != 0){
                            threeCount += 50;
                        }
                    }
                }
                else if(cell[i][j] == piece && cell[i+1][j-1] == piece && cell[i+2][j-2] == piece && cell[i+3][j-3] == 0)
                {
                    threeCount++;
                    if(j-3 == 0 || j-3 == 2 || j-3 == 4)
                    {
                        threeCount += 2;
                        if(j-2 != 0){
                            threeCount += 50;
                        }
                    }
                }
                else if(cell[i][j] == piece && cell[i+1][j-1] == 0 && cell[i+2][j-2] == piece && cell[i+3][j-3] == piece)
                {
                    threeCount++;
                    if(j-1 == 0 || j-1 == 2 || j-1 == 4)
                    {
                        threeCount += 2;
                        if(j != 0){
                            threeCount += 50;
                        }
                    }
                }
                else if(cell[i][j] == piece && cell[i+1][j-1] == piece && cell[i+2][j-2] == 0 && cell[i+3][j-3] == piece)
                {
                    threeCount++;
                    if(j-2 == 0 || j-2 == 2 || j-2 == 4)
                    {
                        threeCount += 2;
                        if(j-1 != 0){
                            threeCount += 50;
                        }
                    }
                }
            }
        }
        
        //diagonal down
        for(int i = 0; i < 4; i++)
        {
            for(int j = 0; j < 3; j++)
            {
                if(cell[i][j] == 0 && cell[i+1][j+1] == piece && cell[i+2][j+2] == piece && cell[i+3][j+3] == piece)
                {
                    threeCount++;
                    if(j == 0 || j == 2 || j == 4)
                    {
                        threeCount += 2;
                        if(j+1 != 0){
                            threeCount += 50;
                        }
                    }
                }
                else if(cell[i][j] == piece && cell[i+1][j+1] == piece && cell[i+2][j+2] == piece && cell[i+3][j+3] == 0)
                {
                    threeCount++;
                    if(j+3 == 0 || j+3 == 2 || j+3 == 4)
                    {
                        threeCount += 2;
                        if(j+4 != 0){
                            threeCount += 50;
                        }
                    }
                }
                else if(cell[i][j] == piece && cell[i+1][j+1] == 0 && cell[i+2][j+2] == piece && cell[i+3][j+3] == piece)
                {
                    threeCount++;
                    if(j+1 == 0 || j+1 == 2 || j+1 == 4)
                    {
                        threeCount += 2;
                        if(j+2 != 0){
                            threeCount += 50;
                        }
                    }
                }
                else if(cell[i][j] == piece && cell[i+1][j+1] == piece && cell[i+2][j+2] == 0 && cell[i+3][j+3] == piece)
                {
                    threeCount++;
                    if(j+2 == 0 || j+2 == 2 || j+2 == 4)
                    {
                        threeCount += 2;
                        if(j+3 != 0){
                            threeCount += 50;
                        }
                    }
                }
            }
        }
    }
    
    return threeCount;
}

int calculateScore(int cell[COLS][ROWS], int piece)
{
    int twoScore = checkTwo(cell, piece);
    int threeScore = checkOddOrEven(cell, piece);
    int fourScore = checkFour(cell, piece);

    int opponentTwoScore = checkTwo(cell, piece == YELLOWPIECE ? REDPIECE : YELLOWPIECE);
    int opponentThreeScore = checkOddOrEven(cell, piece == YELLOWPIECE ? REDPIECE : YELLOWPIECE);
    int opponentFourScore = checkFour(cell, piece == YELLOWPIECE ? REDPIECE : YELLOWPIECE);

    // Adjusted weights to balance offense and defense
    int score = (twoScore * 3) - (opponentTwoScore * 5)
              + (threeScore * 20) - (opponentThreeScore * 30)
              + (fourScore * 500) - (opponentFourScore * 1000);

    return score;
}

void computerMove(int board[COLS][ROWS], int computerPiece)
{
    int bestScore = INT_MIN;
    int bestMoves[COLS]; // to store best columns
    int bestMoveCount = 0;
    
    srand((unsigned)time(NULL) * (unsigned)getpid());

    for (int i = 0; i < COLS; i++)
    {
        if (board[i][0] == 0) // if column is not full
        {
            for (int j = 5; j >= 0; j--)
            {
                if (board[i][j] == 0)
                {
                    board[i][j] = computerPiece;
                    int score = miniMax(board, 6, INT_MIN, INT_MAX, false);
                    board[i][j] = 0;

                    if (score > bestScore)
                    {
                        bestScore = score;
                        bestMoves[0] = i;
                        bestMoveCount = 1;
                    }
                    else if (score == bestScore)
                    {
                        bestMoves[bestMoveCount] = i;
                        bestMoveCount++;
                    }
                    break;
                }
            }
        }
    }

    int chosenColumn = bestMoves[rand() % bestMoveCount];

    for (int j = 5; j >= 0; j--)
    {
        if (board[chosenColumn][j] == 0)
        {
            board[chosenColumn][j] = computerPiece;
            break;
        }
    }
}


bool isTerminalNode(int board[COLS][ROWS])
{
    int notZeroCount = 0;
    for(int i = 0; i < COLS; i++)
    {
        for(int j = 0; j < ROWS; j++)
        {
            if(board[i][j] != 0)
            {
                notZeroCount++;
            }
        }
    }
    if(notZeroCount == 42)
    {
        return true;
    }else if(checkWin(board, 1) == true)
    {
        return true;
    }else if(checkWin(board, 2) == true)
    {
        return true;
    }else
    {
        return false;
    }
}
int max(int x, int y)
{
    if(x > y)
    {
        return x;
    }
    else
    {
        return y;
    }
}
int min(int x, int y)
{
    if(x < y)
    {
        return x;
    }
    else
    {
        return y;
    }
}
int miniMax(int board[COLS][ROWS], int depth, int alpha, int beta, bool isMaximising)
{
    int value = 0;
    bool isTerminal = isTerminalNode(board);
    
    if(depth == 0 || isTerminal)
    {
        if(isTerminal)
        {
            if(checkWin(board, YELLOWPIECE))
            {
                return 1e6;
            }else if(checkWin(board, REDPIECE))
            {
                return - 1e6;
            }else
            {
                return 0;
            }
        }
        else
        {
            return calculateScore(board, YELLOWPIECE);
        }
    }
    if(isMaximising)
    {
        value = INT_MIN;
        
        
        for(int i = 0; i < 7; i++)
        {
            if(board[i][0] == 0)
            {
                for(int j = 5; j >= 0; j--)
                {
                    if(board[i][j] == 0)
                    {
                        board[i][j] = YELLOWPIECE;
                        int newScore = miniMax(board, depth-1, alpha, beta, false);
                        board[i][j] = 0;
                        if(newScore > value)
                        {
                            value = newScore;
                        }
                        alpha = max(alpha, newScore);
                        if(beta <= alpha)
                        {
                            break;
                        }
                        break;
                    }
                }
            }
        }
        return value;
    }
    else
    {
        value = INT_MAX;
        
        for(int i = 0; i < 7; i++)
        {
            if(board[i][0] == 0)
            {
                for(int j = 5; j >= 0; j--)
                {
                    if(board[i][j] == 0)
                    {
                        board[i][j] = REDPIECE;
                        int newScore = miniMax(board, depth-1, alpha, beta, true);
                        board[i][j] = 0;
                        if(newScore < value)
                        {
                            value = newScore;
                        }
                        beta = min(beta, newScore);
                        if(beta <= alpha)
                        {
                            break;
                        }
                        break;
                    }
                }
            }
        }
        return value;
    }
}
