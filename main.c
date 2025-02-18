#include <stdio.h>
#include <stdbool.h>

void quicksort(int list[], int left, int right);
int partition(int list[], int left, int right, int pivot);
void swap(int list[], int left, int right);

// QuickSort function
void quicksort(int arr[], int left, int right) {
    if (left >= right) { // Base case
        return;
    }
    
    int pivot = arr[right]; // Choose pivot
    int partitionIndex = partition(arr, left, right, pivot);
    
    quicksort(arr, left, partitionIndex - 1);  // Sort left partition
    quicksort(arr, partitionIndex + 1, right); // Sort right partition
}

// Partition function
int partition(int list[], int left, int right, int pivot) {
    int leftpointer = left;
    int rightpointer = right - 1;

    while (leftpointer <= rightpointer) {
        while (leftpointer <= rightpointer && list[leftpointer] < pivot) {
            leftpointer++;
        }
        while (rightpointer >= leftpointer && list[rightpointer] > pivot) {
            rightpointer--;
        }

        if (leftpointer < rightpointer) {
            swap(list, leftpointer, rightpointer);
            printf("Items swapped: %d, %d\n", list[leftpointer], list[rightpointer]);
        }
    }

    swap(list, leftpointer, right); // Move pivot to correct position
    printf("Pivot swapped: %d, %d\n", list[leftpointer], list[right]);
    
    return leftpointer;
}

// Swap function
void swap(int list[], int left, int right) {
    int temp = list[left];
    list[left] = list[right];
    list[right] = temp;
}

// Main function
int main() {
    int list[] = {23, 43, 564, 324, 765, 3, 654, 89, 37, 888,
                  900, 304, 100, 875, 12, 9, 1, 45, 69, 31,1234,54321,18,22};
    int size = sizeof(list) / sizeof(list[0]);
    int right = size - 1;

    quicksort(list, 0, right);

    // Print sorted array
    printf("Sorted array:\n");
    for (int i = 0; i < size; i++) {
        printf("%d\n", list[i]);
    }

    return 0;
}

