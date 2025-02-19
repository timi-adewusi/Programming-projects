#include <stdio.h>
#include <stdbool.h>

int main() {
    int num;
    printf("Enter a number less that 20: ");
    scanf("%d", &num);
    
    int i = 1;
    int x = 1;
    while(i <= num)
    {
        x = x * i;
        i++;
    }
    printf("Factorial: %d\n", x);
    return 0;
}

