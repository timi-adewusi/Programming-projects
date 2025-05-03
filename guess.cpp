//
//  main.cpp
//  Wizardry
//
//  Created by Timi Adewusi on 01/05/2025.
//

#include <iostream>
#include <cstdlib>
#include <ctime>

using namespace std;

int main(int argc, const char * argv[]) {
    
    srand(time(0));
    
    bool isGameOver = false;
    
    while(!isGameOver)
    {
        int secretNumber = rand() % 100 + 1;
        int guess = 0;
        int numberOfGuesses = 0;
        
        while(guess != secretNumber)
        {
            cout << "Enter a number" << endl;
            cin >> guess;
            
            if(guess < secretNumber)
            {
                cout << "The number is too low" << endl;
                numberOfGuesses++;
            }
            else if(guess > secretNumber)
            {
                cout << "The number is too high" << endl;
                numberOfGuesses++;
            }
            else
            {
                cout << "You guessed correctly in " << numberOfGuesses << " guesses" << endl;
                numberOfGuesses++;
            }
        }
        cout << "Want to try again? (y/n) " << endl;
        char input;
        
        cin >> input;
        
        switch(input)
        {
            case 'y':
                break;
            default:
                isGameOver = true;
                break;
        }
    }
    return 0;
}
