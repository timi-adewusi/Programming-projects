#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <ctime>
#include <cstdlib>

using namespace std;

void showGrid(SDL_Renderer* renderer, vector<vector<int>>& grid);
void updateGrid(SDL_Renderer* renderer, vector<vector<int>>& grid);
int checkNeighbours(vector<vector<int>>& grid, int row, int column);

int rows = 20;
int cols = 20;


vector<vector<int>> grid(rows, vector<int>(cols, 0));

void showGrid(SDL_Renderer* renderer, vector<vector<int>>& grid)
{
    SDL_Rect cellRect;
    cellRect.w = 20;
    cellRect.h = 20;
    
    for(int i = 0; i < rows; ++i)
    {
        for(int j = 0; j < cols; ++j)
        {
            cellRect.x = j * 40;
            cellRect.y = i * 40;
            
            if(grid[i][j] == 1)
            {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            }
            else
            {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            }
            SDL_RenderFillRect(renderer, &cellRect);
        }
    }
}

int checkNeighbours(vector<vector<int>>& grid, int row, int column)
{
    int neighbours = 0;
    
    //corners
    if(row == 0 && column == 0)
    {
        if(grid[row][column + 1] == 1)
        {
            neighbours++;
        }
        if(grid[row + 1][column] == 1)
        {
            neighbours++;
        }
        if(grid[row + 1][column + 1] == 1)
        {
            neighbours++;
        }
        return neighbours;
    }
    
    if(row == 0 && column == 19)
    {
        if(grid[row][column - 1] == 1)
        {
            neighbours++;
        }
        if(grid[row + 1][column] == 1)
        {
            neighbours++;
        }
        if(grid[row + 1][column - 1] == 1)
        {
            neighbours++;
        }
        return neighbours;
    }
    
    if(row == 19 && column == 0)
    {
        if(grid[row - 1][column] == 1)
        {
            neighbours++;
        }
        if(grid[row][column + 1] == 1)
        {
            neighbours++;
        }
        if(grid[row - 1][column + 1] == 1)
        {
            neighbours++;
        }
        return neighbours;
    }
    
    if(row == 19 && column == 19)
    {
        if(grid[row - 1][column] == 1)
        {
            neighbours++;
        }
        if(grid[row][column - 1] == 1)
        {
            neighbours++;
        }
        if(grid[row - 1][column - 1] == 1)
        {
            neighbours++;
        }
        return neighbours;
    }
    
    //Top edge
    
    if(column > 0 && column < 19 && row < 1)
    {
        if(grid[row][column - 1] == 1){
            neighbours++;
        }
        if(grid[row][column + 1] == 1){
            neighbours++;
        }
        if(grid[row + 1][column - 1] == 1){
            neighbours++;
        }
        if(grid[row + 1][column] == 1){
            neighbours++;
        }
        if(grid[row + 1][column + 1] == 1){
            neighbours++;
        }
        return neighbours;
    }
    
    //Left Edge
    
    if(row > 0 && row < 19 && column < 1)
    {
        if(grid[row - 1][column] == 1)
        {
            neighbours++;
        }
        if(grid[row + 1][column] == 1)
        {
            neighbours++;
        }
        if(grid[row - 1][column + 1] == 1)
        {
            neighbours++;
        }
        if(grid[row][column + 1] == 1)
        {
            neighbours++;
        }
        if(grid[row + 1][column + 1] == 1)
        {
            neighbours++;
        }
        return neighbours;
    }
    
    //Bottom edge
    
    if(column > 0 && column < 19 && row > 18)
    {
        if(grid[row][column - 1] == 1){
            neighbours++;
        }
        if(grid[row][column + 1] == 1){
            neighbours++;
        }
        if(grid[row - 1][column - 1] == 1){
            neighbours++;
        }
        if(grid[row - 1][column] == 1){
            neighbours++;
        }
        if(grid[row - 1][column + 1] == 1){
            neighbours++;
        }
        return neighbours;
    }
    
    //Right Edge
    
    if(row > 0 && row < 19 && column > 8)
    {
        if(grid[row - 1][column] == 1)
        {
            neighbours++;
        }
        if(grid[row + 1][column] == 1)
        {
            neighbours++;
        }
        if(grid[row - 1][column - 1] == 1)
        {
            neighbours++;
        }
        if(grid[row][column - 1] == 1)
        {
            neighbours++;
        }
        if(grid[row + 1][column - 1] == 1)
        {
            neighbours++;
        }
        return neighbours;
    }
    
    //center
    if(row > 0 && row < 19 && column > 0 && column < 19)
    {
        if(grid[row - 1][column] == 1)
        {
            neighbours++;
        }
        if(grid[row][column - 1] == 1)
        {
            neighbours++;
        }
        if(grid[row - 1][column - 1] == 1)
        {
            neighbours++;
        }
        if(grid[row + 1][column] == 1)
        {
            neighbours++;
        }
        if(grid[row][column + 1] == 1)
        {
            neighbours++;
        }
        if(grid[row + 1][column + 1] == 1)
        {
            neighbours++;
        }
        if(grid[row - 1][column + 1] == 1)
        {
            neighbours++;
        }
        if(grid[row + 1][column - 1] == 1)
        {
            neighbours++;
        }
        return neighbours;
    }
    return neighbours;
}
void updateGrid(SDL_Renderer* renderer, vector<vector<int>>& grid)
{
    
    vector<vector<int>> TempGrid(rows, vector<int>(cols, 0));
    
    for(int i = 0; i < rows; ++i)
    {
        for(int j = 0; j < cols; ++j)
        {
            TempGrid[i][j] = 0;
        }
    }
    for(int i = 0; i < rows; ++i)
    {
        for(int j = 0; j < cols; ++j)
        {
            int numberOfNeighbours = checkNeighbours(grid, i, j);
            
            if(grid[i][j] == 1 && numberOfNeighbours < 2)
            {
                TempGrid[i][j] = 0;
            }
            else if(grid[i][j] == 1 && (numberOfNeighbours == 2 || numberOfNeighbours == 3))
            {
                TempGrid[i][j] = 1;
            }
            else if(grid[i][j] == 1 && numberOfNeighbours > 3)
            {
                TempGrid[i][j] = 0;
            }
            else if(grid[i][j] == 0 && numberOfNeighbours == 3)
            {
                TempGrid[i][j] = 1;
            }
        }
    }
    for(int i = 0; i < rows; ++i)
    {
        for(int j = 0; j < cols; ++j)
        {
            grid[i][j] = TempGrid[i][j];
        }
    }
}

int main() {
    
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        cout << "SDL could not be initialised: " << SDL_GetError();
        return 1;
    }
    
    SDL_Window* window = SDL_CreateWindow("C++ SDL2 window", 100, 100 , 800, 800, SDL_WINDOW_SHOWN);
    
    if(!window)
    {
        cout << "SDL_CreateWindow Error: " << SDL_GetError() << endl;
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    if(!renderer)
    {
        cout << "SDL_CreateRenderer Error: " << SDL_GetError() << endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    srand(time(0));
    
    for(int i = 0; i < rows; ++i)
    {
        for(int j = 0; j < cols; ++j)
        {
            int binary = rand() % 2;
            grid[i][j] = binary;
        }
    }
    
    bool running = true;
    SDL_Event event;
    
    while(running)
    {
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT)
            {
                running = false;
            }
        }
        
        updateGrid(renderer, grid);
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        showGrid(renderer, grid);
        
        SDL_RenderPresent(renderer);
        
        SDL_Delay(500);
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    
    SDL_Quit();
    return 0;
}

