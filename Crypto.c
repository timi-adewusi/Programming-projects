#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

struct loginInfo{
    char userName[50];
    char password[50];
    int phoneNumber;
};

struct bankBalance{
    int balance;
};

struct cryptos{
   char name[20];
   int value;
   int amount;
};

struct cryptoWallet{
    char name[20];
    int amount;
};

struct cryptos cryptoList[5] = {
    {"Bitcoin", 30000, 0},
    {"Ethereum", 2000, 0},
    {"Litecoin", 150, 0},
    {"Ripple", 1, 0},
    {"Cardano", 1, 0}
};

struct cryptoWallet userWallet[5] = {
    {"Bitcoin", 0},
    {"Ethereum", 0},
    {"Litecoin", 0},
    {"Ripple", 0},
    {"Cardano", 0}
};

void deposit(struct bankBalance *userBalance, int amount);
void withdraw(struct bankBalance *userBalance, int amount);
void buyCrypto(struct cryptos cryptoList[], int cryptoCount, struct bankBalance *userBalance, struct cryptoWallet *userWallet, int cryptoIndex, int amount);
void sellCrypto(struct cryptos cryptoList[], int cryptoCount, struct bankBalance *userBalance, struct cryptoWallet *userWallet, int cryptoIndex, int amount);
void exchangeCrypto(struct cryptos cryptoList[], int cryptoCount, struct cryptoWallet *userWallet, int fromIndex, int toIndex, int amount);
void viewBalance(struct bankBalance *userBalance);
void viewCryptoWallet(struct cryptoWallet userWallet[], int cryptoCount);
void cryptoValueChange(struct cryptos cryptoList[], int cryptoCount);
void cryptoApp(bool loggedIn, struct bankBalance *userBalance);
void menu(void);

void cryptoValueChange(struct cryptos cryptoList[], int cryptoCount) {
    srand(time(0));
    for (int i = 0; i < cryptoCount; i++) {
        double percentChange = ((rand() % 601) - 300) / 10000.0;
        int change = (int)(cryptoList[i].value * percentChange);
        if (change == 0) change = (percentChange > 0) ? 1 : -1;
        if ((rand() % 100) < 5) {
            int spike = (rand() % 2001) - 1000;
            change += spike;
        }
        cryptoList[i].value += change;
        if (cryptoList[i].value < 1) cryptoList[i].value = 1;
    }
}

void deposit(struct bankBalance *userBalance, int amount){
    userBalance->balance += amount;
    printf("Deposit successful. New balance: %d\n", userBalance->balance);
}

void withdraw(struct bankBalance *userBalance, int amount){
    if(amount > userBalance->balance){
        printf("Insufficient funds\n");
    } else {
        userBalance->balance -= amount;
        printf("Withdrawal successful. New balance: %d\n", userBalance->balance);
    }
}

void buyCrypto(struct cryptos cryptoList[], int cryptoCount, struct bankBalance *userBalance, struct cryptoWallet *userWallet, int cryptoIndex, int amount){
    if(cryptoIndex < 0 || cryptoIndex >= cryptoCount){
        printf("Invalid crypto selection.\n");
        return;
    }
    int cost = cryptoList[cryptoIndex].value * amount;
    if(cost > userBalance->balance){
        printf("Insufficient funds to buy %d of %s.\n", amount, cryptoList[cryptoIndex].name);
        return;
    }
    userBalance->balance -= cost;
    userWallet[cryptoIndex].amount += amount;
    printf("Successfully bought %d of %s. New balance: %d\n", amount, cryptoList[cryptoIndex].name, userBalance->balance);
}

void sellCrypto(struct cryptos cryptoList[], int cryptoCount, struct bankBalance *userBalance, struct cryptoWallet *userWallet, int cryptoIndex, int amount){
    if(cryptoIndex < 0 || cryptoIndex >= cryptoCount){
        printf("Invalid crypto selection.\n");
        return;
    }
    if(amount > userWallet[cryptoIndex].amount){
        printf("Insufficient %s in wallet to sell.\n", cryptoList[cryptoIndex].name);
        return;
    }
    int revenue = cryptoList[cryptoIndex].value * amount;
    userBalance->balance += revenue;
    userWallet[cryptoIndex].amount -= amount;
    printf("Successfully sold %d of %s. New balance: %d\n", amount, cryptoList[cryptoIndex].name, userBalance->balance);
}

void viewBalance(struct bankBalance *userBalance){
    printf("Current balance: %d\n", userBalance->balance);
}

void viewCryptoWallet(struct cryptoWallet userWallet[], int cryptoCount){
    printf("Crypto Wallet:\n");
    for(int i = 0; i < cryptoCount; i++){
        if(userWallet[i].amount > 0){
            printf("%s: %d\n", userWallet[i].name, userWallet[i].amount);
        }
    }
}

void exchangeCrypto(struct cryptos cryptoList[], int cryptoCount, struct cryptoWallet *userWallet, int fromIndex, int toIndex, int amount){
    if(fromIndex < 0 || fromIndex >= cryptoCount || toIndex < 0 || toIndex >= cryptoCount){
        printf("Invalid crypto selection.\n");
        return;
    }
    if(amount > userWallet[fromIndex].amount){
        printf("Insufficient %s in wallet to exchange.\n", cryptoList[fromIndex].name);
        return;
    }
    int fromValue = cryptoList[fromIndex].value * amount;
    int toAmount = fromValue / cryptoList[toIndex].value;
    userWallet[fromIndex].amount -= amount;
    userWallet[toIndex].amount += toAmount;
    printf("Successfully exchanged %d of %s to %d of %s.\n", amount, cryptoList[fromIndex].name, toAmount, cryptoList[toIndex].name);
}

void cryptoApp(bool loggedIn, struct bankBalance *userBalance){
    while(loggedIn){
        menu();
        cryptoValueChange(cryptoList, 5);
        int choice;
        scanf("%d", &choice);
        switch(choice){
            case 1: {
                printf("Buying crypto...\n");
                printf("Enter crypto index (0-4) and amount: ");
                int cryptoIndex, amount;
                scanf("%d %d", &cryptoIndex, &amount);
                buyCrypto(cryptoList, 5, userBalance, userWallet, cryptoIndex, amount);
                break;
            }
            case 2: {
                printf("Exchanging crypto...\n");
                printf("Enter from crypto index (0-4), to crypto index (0-4), and amount: ");
                int fromIndex, toIndex, exchangeAmount;
                scanf("%d %d %d", &fromIndex, &toIndex, &exchangeAmount);
                exchangeCrypto(cryptoList, 5, userWallet, fromIndex, toIndex, exchangeAmount);
                break;
            }
            case 3: {
                printf("Withdrawing money...\n");
                printf("Enter amount to withdraw: ");
                int withdrawAmount;
                scanf("%d", &withdrawAmount);
                withdraw(userBalance, withdrawAmount);
                break;
            }
            case 4: {
                printf("Selling crypto...\n");
                printf("Enter crypto index (0-4) and amount: ");
                int sellCryptoIndex, sellAmount;
                scanf("%d %d", &sellCryptoIndex, &sellAmount);
                sellCrypto(cryptoList, 5, userBalance, userWallet, sellCryptoIndex, sellAmount);
                break;
            }
            case 5: {
                printf("Depositing funds...\n");
                printf("Enter amount to deposit: ");
                int depositAmount;
                scanf("%d", &depositAmount);
                deposit(userBalance, depositAmount);
                break;
            }
            case 6:
                viewBalance(userBalance);
                break;
            case 7:
                viewCryptoWallet(userWallet, 5);
                break;
            case 8:
                loggedIn = false;
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }
}

void menu(){
    printf("--------------------------Menu--------------------------\n");
    printf("1. Buy crypto \n");
    printf("2. Exchange crypto \n");
    printf("3. Withdraw money from crypto wallet \n");
    printf("4. Sell crypto \n");
    printf("5. Deposit funds \n");
    printf("6. View balance \n");
    printf("7. View crypto wallet \n");
    printf("8. Exit \n");
}

int main(){
    struct loginInfo userDetails;
    struct bankBalance userBalance = {.balance = 1000};

    printf("Enter your username: \n");
    fgets(userDetails.userName, sizeof(userDetails.userName), stdin);

    printf("You typed: %s", userDetails.userName);

    printf("Enter your password: \n");
    fgets(userDetails.password, sizeof(userDetails.password), stdin);

    printf("You typed: %s", userDetails.password);

    printf("\n");

    bool loggedIn = true;

    cryptoApp(loggedIn, &userBalance);
    return 0;
}
