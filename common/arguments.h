#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    int number_of_groups;
    int number_of_areas;
} arguments;

bool is_argc_correct(const arguments *args) {
    if (args->number_of_areas <= 0) {
        return false;
    }

    if (args->number_of_groups <= 0) {
        return false;
    }

    return args->number_of_areas > args->number_of_groups;
}

bool try_parse_arguments(int argc, const char *argv[], arguments *out) {
    if (argc != 3) {
        return false;
    }

    int number_of_groups = atoi(argv[1]);

    if (number_of_groups == 0) {
        return false;
    }

    int number_of_areas = atoi(argv[2]);

    if (number_of_areas == 0) {
        return false;
    }

    out->number_of_groups = number_of_groups;
    out->number_of_areas = number_of_areas;
    return true;
}

