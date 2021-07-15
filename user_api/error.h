#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define error(fmt, args ...) do { fprintf(stderr, "ERROR %s:"fmt, __FUNCTION__, ## args); \
				  fprintf(stderr, errno ? ":%m\n" : "\n"); \
				  exit(EXIT_FAILURE); } while (0)
#define debug(fmt, args ...) fprintf(stderr, "%s:%d:"fmt "\n", __FILE__, __LINE__, ## args)
