#include <time.h>

time_t rfc822_to_seconds(const char *rfc822);
size_t seconds_to_rfc822(const time_t time, char *rfc822_out, size_t maxsize);
time_t now();
