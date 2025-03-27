let nums: number[] = [2, 7, 11, 15];
let target = 18;

function twoSum(nums: number[], target: number): [number, number]{
    let numberToSearch = 0;

    for(let num = 0; num < nums.length; num++){
        if(nums[num] < target){
            numberToSearch = target - nums[num]

            for(let y = 0; y < nums.length; y++){
                if(y != num && nums[y] == numberToSearch){
                    return [num, y];
                }
            }
        }
    }
};

const numFunction = twoSum(nums, target);
console.log(numFunction);
