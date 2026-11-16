#pragma once

#include <stdbool.h>
#include <stddef.h>

bool relay_command_parse(const char *body, size_t length, bool *on);
