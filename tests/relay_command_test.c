#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "relay_command.h"

int main(void)
{
    bool on = false;
    assert(relay_command_parse("true", 4, &on) && on);
    assert(relay_command_parse("false", 5, &on) && !on);
    assert(relay_command_parse(" \ntrue\t", 7, &on) && on);
    assert(relay_command_parse("\rfalse ", 7, &on) && !on);

    const char *invalid[] = {"", " ", "TRUE", "False", "1", "0", "null", "on",
                             "\"true\"", "{\"on\":true}", "tru", "falsefalse", "true false"};
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        on = true;
        assert(!relay_command_parse(invalid[i], strlen(invalid[i]), &on));
        assert(on);
    }
    assert(!relay_command_parse("true\0false", 10, &on));
    assert(!relay_command_parse(NULL, 4, &on));
    assert(!relay_command_parse("true", 4, NULL));
    puts("Relay command tests passed.");
    return 0;
}
