import promptSync from 'prompt-sync';

const prompt = promptSync();

function getRandomInt(max: number) {
    return Math.floor(Math.random() * max) + 1;
}

function shop(): void {

}

type Weapons = {
    name: string;
    coins: number;
    damage: number;
}

let weapons: Weapons[] = [
    { name: "Bomb", coins: 25, damage: 25 },
    { name: "Sling shot", coins: 10, damage: 10 },
    { name: "Bow and arrow", coins: 15, damage: 15 },
    { name: "Gun", coins: 20, damage: 30 },
    { name: "Sword", coins: 60, damage: 50 }

];

type Person = {
    healthPoints: number;
    attackDamage: number;
    coins: number;
};

const person: Person = {
    healthPoints: 1000,
    attackDamage: 50,
    coins: 10,
}

type Enemy = {
    name: string;
    healthPoints: number;
    attackDamage: number;
    isAlive: boolean;
};

const zombies: Enemy[] = [];

const wolves: Enemy[] = [];

let rounds: number = 1;

let maxRounds: number = 1;

console.log("This is a text adventure game");
console.log("Your mission is to defeat all the zombies and wolves");
console.log("You can collect coins for upgrades on your journey");
console.log("Press space to attack");
console.log("Good luck!");
console.log("");

while (rounds <= maxRounds) {

    let numberOfZombies: number = 0, numberOfWolves = 0;

    if (rounds == 1) {
        numberOfZombies = getRandomInt(3);
        numberOfWolves = getRandomInt(3);
    }
    else if (rounds == 2) {
        numberOfZombies = getRandomInt(4);
        numberOfWolves = getRandomInt(4);
    }
    else if (rounds == 3) {
        numberOfZombies = getRandomInt(5);
        numberOfWolves = getRandomInt(5);
    }
    else if (rounds == 4) {
        numberOfZombies = getRandomInt(7);
        numberOfWolves = getRandomInt(7);
    }
    else {
        numberOfZombies = getRandomInt(10);
        numberOfWolves = getRandomInt(10);
    }

    console.log("There are " + numberOfZombies + " zombies");
    console.log("There are " + numberOfWolves + " wolves");

    while (numberOfWolves > 0 || numberOfZombies > 0) {

        for (let i = 0; i < numberOfZombies; i++) {
            zombies.push({ name: "Zombie", healthPoints: 100, attackDamage: 8, isAlive: true });
        }

        for (let i = 0; i < numberOfWolves; i++) {
            wolves.push({ name: "wolf", healthPoints: 150, attackDamage: 12, isAlive: true });
        }

        if (numberOfZombies >= 1) {
            let temp = numberOfZombies - 1;

            const input = prompt("Pick a zombie from 0 to " + temp);

            let zombiePicked: number = Number(input);

            const attack = prompt("Attack!");

            if (attack === ' ') {
                zombies[zombiePicked].healthPoints -= person.attackDamage;

                if(zombies[zombiePicked].healthPoints < 0)
                {
                    console.log("You are attacking a dead zombie!");
                }
                else
                {
                    console.log("The zombie has: " + zombies[zombiePicked].healthPoints + " healthpoints");
                }

                let numberOfZombieAttacks: number = getRandomInt(numberOfZombies);
                let numberOfWolfAttacks: number = getRandomInt(numberOfWolves);

                let totalZombieDamage: number = zombies[zombiePicked].attackDamage * numberOfZombieAttacks;
                let totalWolfDamage: number = 12 * numberOfWolfAttacks;

                if (numberOfWolves == 0) {
                    totalWolfDamage = 0;
                }

                let completeDamage: number = totalWolfDamage + totalZombieDamage;

                person.healthPoints -= completeDamage;

                console.log("You have: " + person.healthPoints + " healthpoints");
            }
        }
        if (numberOfWolves >= 1) {
            let temp2 = numberOfWolves - 1;

            const input2 = prompt("Pick a wolf from 0 to " + temp2);

            let wolfPicked: number = Number(input2);

            const attackWolf = prompt("Attack!");

            if (attackWolf === ' ') {
                wolves[wolfPicked].healthPoints -= person.attackDamage;

                if(wolves[wolfPicked].healthPoints < 0)
                {
                    console.log("You are attacking a dead wolf!");
                }
                else
                {
                    console.log("The wolf has: " + wolves[wolfPicked].healthPoints + " healthpoints");
                }
                let numberOfZombieAttacks: number = getRandomInt(numberOfZombies);
                let numberOfWolfAttacks: number = getRandomInt(numberOfWolves);

                let totalZombieDamage: number = 8 * numberOfZombieAttacks;
                let totalWolfDamage: number = 12 * numberOfZombieAttacks;

                if (numberOfZombies == 0) {
                    totalZombieDamage = 0;
                }

                let completeDamage: number = totalWolfDamage + totalZombieDamage;

                person.healthPoints -= completeDamage;

                console.log("You have: " + person.healthPoints + " healthpoints");
            }
        }

        for (let i = 0; i < numberOfZombies; i++) {
            if (zombies[i].healthPoints <= 0) {
                zombies[i].isAlive = false;
                zombies.splice(i, 1);
                numberOfZombies -= 1;
            }
        }

        for (let i = 0; i < numberOfWolves; i++) {
            if (wolves[i].healthPoints <= 0) {
                wolves[i].isAlive = false;
                wolves.splice(i, 1);
                numberOfWolves -= 1;
            }
        }
    }
    rounds++;
}
