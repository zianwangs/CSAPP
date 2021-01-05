#include <stdio.h>

void quicksort(int *, int);
void partition(int *, int, int);
void swap(int *, int, int);

int main() {
    int nums[] = {4,5,6,6,7,3,2,2,1};
    for (int i = 0; i < 9; ++i) printf("%d ", nums[i]);
    printf("\n");
    quicksort(nums, 9);
    for (int i = 0; i < 9; ++i) printf("%d ", nums[i]);
}

void quicksort(int * nums, int len) {
    partition(nums, 0, len - 1);
}

void partition(int * nums, int left, int right) {
    if (left >= right) return;
    int pivot = nums[left];
    int lo = left + 1, hi = right;
    while (lo <= hi) {
        if (nums[lo] > pivot) swap(nums, lo, hi--);
        else ++lo;
    }
    swap(nums, left, hi);
    partition(nums, left, hi - 1);
    partition(nums, hi + 1, right);
}

void swap(int * nums, int left, int right) {
    if (left == right) return;
    int temp = nums[left];
    nums[left] = nums[right];
    nums[right] = temp;
}

