#ifndef PARSING_H
#define PARSING_H
#define MAX_ARGUMENT 15

#include "defs.h"
#include "types.h"
#include "createcmd.h"
#include "utils.h"

struct cmd *parse_line(char *b);

#endif  // PARSING_H
