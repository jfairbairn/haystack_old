#include <xlocale.h>
#include <sys/time.h>
#include "timeutil.h"

// HTTP date: Wed, 26 Feb 2014 07:55:15 GMT
#define RFC822_FORMAT "%a, %d %b %Y %T %Z"
#define RFC822_FORMAT_OUT "%a, %d %b %Y %T GMT"
time_t rfc822_to_seconds(const char *rfc822)
{
	struct tm tm;
	strptime(rfc822, RFC822_FORMAT, &tm);
	return mktime(&tm);
}

size_t seconds_to_rfc822(const time_t time, char *rfc822_out, size_t maxsize)
{
	struct tm tm;
	gmtime_r(&time, &tm);
	return strftime(rfc822_out, maxsize, RFC822_FORMAT_OUT, &tm);
}

time_t now()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return tp.tv_sec;
}
