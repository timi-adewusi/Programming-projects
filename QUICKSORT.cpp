//
//  main.cpp
//  Wizardry
//
//  Created by Timi Adewusi on 01/05/2025.
//

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <vector>

using namespace std;

void quickSort(vector<int>& arr, int low, int high);
int partition(vector<int>& arr, int low, int high);

void quickSort(vector<int>& arr, int low, int high)
{
    if(low < high)
    {
        int pivotIndex = partition(arr, low, high);
        quickSort(arr, low, pivotIndex - 1);
        quickSort(arr, pivotIndex + 1, high);
    }
}

int partition(vector<int>& arr, int low, int high)
{
    int pivot = arr[high];
    int i = low - 1;
    for(int j = low; j <= high - 1; j++)
    {
        if(arr[j] < pivot)
        {
            i = i + 1;
            int temp;
            temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }
    int temp2;
    temp2 = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = temp2;
    
    return i + 1;
}

int main(int argc, const char * argv[])
{
    vector<int> list(10);
    list = {12,65,232,43,2234,98,34,23,100,88};
    
    for(int i = 0; i < 10; i++)
    {
        cout << list[i] << " ";
    }
    cout << endl;
    
    quickSort(list, 0, 9);
    
    for(int i = 0; i < 10; i++)
    {
        cout << list[i] << " ";
    }
    cout << endl;
    return 0;
}
