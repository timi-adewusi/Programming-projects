import promptSync from 'prompt-sync';

const prompt = promptSync();

function getRandomInt(max: number)
{
    return Math.floor(Math.random() * max);
}

function sum(number1: number, number2: number)
{
    return number1 + number2;
}

const questions: number = 5;
let i: number = 1;
let answersCorrect: number = 0;

while(i <= questions)
{
    let num1 = getRandomInt(21);
    let num2 = getRandomInt(21);

    let sumOfNumbers: number = sum(num1, num2);

    const question = prompt(num1 + " + " + num2 + ": ");
    const answer = Number(question);

    if(answer == sumOfNumbers)
    {
        console.log("Correct! The answer is " + sumOfNumbers);
        answersCorrect++;
    }
    else
    {
        console.log("Incorrect! The answer is " + sumOfNumbers);
    }
    i++;
}

console.log("You got " + answersCorrect + " questions correct!");
