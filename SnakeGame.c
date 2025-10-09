#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

void initInput(void) {
    struct termios settings;
    tcgetattr(STDIN_FILENO, &settings);
    settings.c_lflag &= ~(ICANON | ECHO); // Disable buffering and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &settings);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK); // Non-blocking input
}

char getInput(void) {
    char ch = 0;
    read(STDIN_FILENO, &ch, 1);
    return ch;
}

void clearScreen(void);

void clearScreen(void) {
    for (int i = 0; i < 30; i++) {
        printf("\n");
    }
}

void waiting(int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

int grid[10][10];

int score = 0;

int gridLength = 10;

struct turningPoint{
    int x;
    int y;
};

struct position{
    int x;
    int y;
};
struct food{
    int x;
    int y;
    int symbol;
};
struct piece{
    struct position pos;
    int symbol;
    char direction;
};

struct snake{
    struct piece pieces[100];
    int length;
    char direction;
};
struct turningPoint point;
struct snake player;
struct food apple;

void generateApple(int grid[10][10]){
    srand(time(0));
    int xPosition = rand() % 10;
    int yPosition = rand() % 10;
    
    apple.x = xPosition;
    apple.y = yPosition;
}

void updateGrid(int grid[10][10]) {
    // Clear the grid first
    for (int i = 0; i < gridLength; i++) {
        for (int j = 0; j < gridLength; j++) {
            grid[i][j] = 0;
        }
    }
    
    grid[apple.y][apple.x] = 2;
    //If the head eats the apple
    if(player.pieces[player.length - 1].pos.x == apple.x && player.pieces[player.length - 1].pos.y == apple.y){
        score += 1;
        player.length += 1;
        
        //shifts the segments and adds a new one at a new tail
        for(int i = player.length - 1; i > 0; i--){
            player.pieces[i].pos.x = player.pieces[i - 1].pos.x;
            player.pieces[i].pos.y = player.pieces[i - 1].pos.y;
            player.pieces[i].direction = player.pieces[i - 1].direction;
        }
        if(player.pieces[1].direction == 'r'){
            player.pieces[0].pos.x = player.pieces[1].pos.x - 1;
            player.pieces[0].direction = 'r';
        }
        else if(player.pieces[1].direction == 'l'){
            player.pieces[0].pos.x = player.pieces[1].pos.x + 1;
            player.pieces[0].direction = 'l';
        }
        else if(player.pieces[1].direction == 'd'){
            player.pieces[0].pos.y = player.pieces[1].pos.y - 1;
            player.pieces[0].direction = 'd';
        }
        else if(player.pieces[1].direction == 'u'){
            player.pieces[0].pos.y = player.pieces[1].pos.y + 1;
            player.pieces[0].direction = 'u';
        }
        generateApple(grid);
    }
    
    // Propagate directions from tail to head
    for (int i = 0; i < player.length - 1; i++) {
        player.pieces[i].direction = player.pieces[i + 1].direction;
    }

    // Set the head's direction to the latest player input
    player.pieces[player.length - 1].direction = player.direction;

    // Move each piece based on its direction
    for (int i = 0; i < player.length; i++) {
        switch (player.pieces[i].direction) {
            case 'r': player.pieces[i].pos.x++; break;
            case 'l': player.pieces[i].pos.x--; break;
            case 'u': player.pieces[i].pos.y--; break;
            case 'd': player.pieces[i].pos.y++; break;
        }
    }

    // Update the grid with new snake positions
    for (int i = 0; i < player.length; i++) {
        int x = player.pieces[i].pos.x;
        int y = player.pieces[i].pos.y;
        if (x >= 0 && x < gridLength && y >= 0 && y < gridLength) {
            grid[y][x] = 1;
        }
    }
}

void displayGrid(int grid[10][10]){
    for(int i = 0; i < gridLength; i++){
        for(int j = 0; j < gridLength; j++){
            if(grid[i][j] == 1){
                printf("0");
            }
            else if(grid[i][j] == 0){
                printf("-");
            }
            else{
                printf("@");
            }
        }
        printf("\n");
    }
    printf("Score: %d", score);
}

void userInput(void){
    char input = getInput();
    
    if(input == 'a'){
        player.direction = 'l';
    }
    else if(input == 'd'){
        player.direction = 'r';
    }
    else if(input == 'w'){
        player.direction = 'u';
    }
    else if(input == 's'){
        player.direction = 'd';
    }
    
    point.x = player.pieces[player.length - 1].pos.x;
    point.y = player.pieces[player.length - 1].pos.y;
}

bool collision(int grid[10][10]){
    for(int i = 0; i < player.length - 2; i++){
        if(player.pieces[player.length - 1].pos.x == player.pieces[i].pos.x && player.pieces[player.length - 1].pos.y == player.pieces[i].pos.y){
            return true;
        }
    }
    
    int xhead = player.pieces[player.length - 1].pos.x;
    int yhead = player.pieces[player.length - 1].pos.y;
    
    if(xhead > 9 || xhead < 0 || yhead > 9 || yhead < 0){
        return true;
    }
    
    return false;
}

void gameLoop(void){
    bool isRunning = true;
    
    while(isRunning){
        userInput();
        displayGrid(grid);
        waiting(200);
        clearScreen();
        updateGrid(grid);
        
        if(collision(grid)){
            isRunning = false;
        }
    }
}

void startGame(void){
    for(int i = 0; i < gridLength; i++){
        for(int j = 0; j < gridLength; j++){
            grid[i][j] = 0;
        }
    }
    
    int x = 0;
    int y = 0;
    
    player.pieces[0].pos.x = x;
    player.pieces[0].pos.y = y;
    
    grid[y][x] = 0;
    
    char direction = 'r';
    player.direction = direction;
    
    for(int i = 0; i < player.length; i++){
        player.pieces[i].direction = 'r';
    }
    
    player.length = 5;
    
    generateApple(grid);
}

int main() {
    initInput();
    startGame();
    gameLoop();
    return 0;
}

