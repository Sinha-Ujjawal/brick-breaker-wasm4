#include <stdbool.h>

#ifndef UTILS_H_
#define UTILS_H_

// Clamps a give integer between min_x and max_x
int clamp_int(int x, int min_x, int max_x) {
    if (x < min_x) {
        return min_x;
    }
    if (x > max_x) {
        return max_x;
    }
    return x;
}

// Checks if two lines are overlapping
// Case 1:
// l1 ----- h1
//      l2 ----- h2
// Case 2:
//      l1 ----- h1
// l2 ----- h2
// Other cases are non-overlapping
bool overlap(int l1, int h1, int l2, int h2) {
    return (l1 <= h2) && (l2 <= h1);
}


// A lightweight custom function to reverse a string
void reverse(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = *(str + start);
        *(str + start) = *(str + end);
        *(str + end) = temp;
        start++;
        end--;
    }
}

// A lightweight custom implementation of itoa
void itoa(int value, char* buffer, int base) {
    int i = 0;
    int isNegative = 0;

    // Handle 0 explicitly, otherwise empty string is printed
    if (value == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }

    // Handle negative numbers
    if (value < 0 && base == 10) {
        isNegative = 1;
        value = -value;
    }

    // Process individual digits
    while (value != 0) {
        int rem = value % base;
        buffer[i++] = (char) ((rem > 9) ? (rem - 10 + 'a') : (rem + '0'));
        value = value / base;
    }

    // Append negative sign
    if (isNegative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0'; // Null-terminate the string

    // Reverse the string
    reverse(buffer, i);
}

#endif
