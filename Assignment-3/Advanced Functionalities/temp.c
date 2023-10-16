#include <stdio.h>
#include <string.h>

#define MAX_STRINGS 10000
#define MAX_STRING_LENGTH 100

int max_priority(int priorities[], int size) {
    int max_val = -1;
    int max_idx = -1;

    for (int i = 0; i < size; i++) {
        if (priorities[i] != -1 && (max_idx == -1 || priorities[i] > max_val)) {
            max_val = priorities[i];
            max_idx = i;
        }
    }

    return max_idx;
}

void priority(char strings[][MAX_STRING_LENGTH], int priorities[], int size, char result[MAX_STRINGS][MAX_STRING_LENGTH]) {
    int i = 0;

    while (i < MAX_STRINGS) {
        int max_idx = max_priority(priorities, size);

        if (max_idx == -1) {
            break;
        }

        priorities[max_idx] = -1;
        strcpy(result[i], strings[max_idx]);
        i++;
    }
}

int main() {
    char strings[MAX_STRINGS][MAX_STRING_LENGTH] = {
        "job1",
        "job2",
        "job3",
        "job4",
        "job5"
    };
    int priorities[MAX_STRINGS] = {5,4,7,1,2};

    char result[MAX_STRINGS][MAX_STRING_LENGTH];
    int size = MAX_STRINGS;
    priority(strings, priorities, size, result);

    for (int i = 0; i < MAX_STRINGS; i++) {
        if (result[i][0] == '\0') {
            break;
        }
        printf("%s\n", result[i]);
    }

    return 0;
}
