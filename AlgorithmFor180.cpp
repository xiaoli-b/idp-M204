#include <iostream>
using namespace std;

enum Direction { north, east, south, west };
Direction current_direction = north;
int current_node = 2;


int rot[] = { 3, 2, -1, 2, 1, 3, 0, 0, 0, 1 };
int turns[] = { 2, 2, 1, 1 };

void reverse_array(int arr[], int start, int end) {
    while (start < end) {
        std::swap(arr[start], arr[end]);
        ++start;
        --end;
    }
}

void rotate_array_right(int arr[], int n, int d) {
    d = d % n;
    // Reverse the entire array
    reverse_array(arr, 0, n - 1);
    // Reverse the first 'd' elements
    reverse_array(arr, 0, d - 1);
    // Reverse the remaining elements
    reverse_array(arr, d, n - 1);
}


int get_turns() {
    int num_of_rots = rot[current_node];
    int n = sizeof(turns) / sizeof(turns[0]);
    int d = num_of_rots;
    rotate_array_right(turns, n, d);
    int turns_needed = turns[current_direction];
    if (current_node == 2) {
        return 2;
    }
    return turns_needed;
}

int main() {
    int turns_needed = get_turns();
    std::cout << turns_needed << std::endl;

    return 0;
}