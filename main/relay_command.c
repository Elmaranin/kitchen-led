#include "relay_command.h"

#include <string.h>

static bool json_space(char value)
{
    return value == ' ' || value == '\t' || value == '\r' || value == '\n';
}

bool relay_command_parse(const char *body, size_t length, bool *on)
{
    if (body == NULL || on == NULL) {
        return false;
    }
    while (length > 0 && json_space(*body)) {
        ++body;
        --length;
    }
    while (length > 0 && json_space(body[length - 1])) {
        --length;
    }
    if (length == 4 && memcmp(body, "true", 4) == 0) {
        *on = true;
        return true;
    }
    if (length == 5 && memcmp(body, "false", 5) == 0) {
        *on = false;
        return true;
    }
    return false;
}
