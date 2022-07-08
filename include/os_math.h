#pragma once

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define IS_POW2_OR_ZERO(x) (((x) & ((x)-1)) == 0)
