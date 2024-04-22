#pragma once

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

bool parse_help(int argc, const char *argv[]) {
    return argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0);
}

void print_help(const char *exec_name) {
    printf("Usage: %s <number_of_groups> <number_of_areas>\n", exec_name);
}