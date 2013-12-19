#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/errno.h>
#include <getopt.h>
#include <inttypes.h>

#import <google/sparse_hash_map>
#import <google/dense_hash_map>

void http_cb(struct evhttp_request *request, void *data) {
	int fd = dup(*((int* )data));
	struct stat st;
	fstat(fd, &st);

	if (fd == -1)
	{
		evhttp_send_error(request, HTTP_NOTFOUND, NULL);
	}
	else
	{
		off_t off = lseek(fd, 0, SEEK_SET);
		if (off != 0)
		{

			fprintf(stderr, "lseek returned %lld\n for fd %d: %s", off, fd, strerror(errno));
			evhttp_send_error(request, HTTP_INTERNAL, NULL);
			goto end;
		}
		evbuffer *buf = evbuffer_new();
		evbuffer_add_file(
			buf,
			fd, 0, st.st_size
		);
		evhttp_add_header(
			evhttp_request_get_output_headers(request),
			"Content-Type",
			"text/plain"
		);
		evhttp_send_reply(request, HTTP_OK, NULL, buf);
		evbuffer_free(buf);
	}
	end:
	return;
}

int main(int argc, char * const *argv) {

	static struct option long_options[] =
	{
		{"port", required_argument, 0, 'p'},
		{"file", required_argument, 0, 'f'}
	};

	int option_index = 0;
	int c;
	int portnum = -1;
	int fd = -2;
	while (1) {

		c = getopt_long(argc, argv, "p:f:", long_options, &option_index);
		if (c == -1) break;

		switch (c)
		{
			case 'f':
				fd = open(optarg, O_RDONLY);
				break;
			case 'p':
				portnum = (int) strtol(optarg, NULL, 10);
				break;
		}

	}

	if (portnum < 0 || fd < 0) {
		exit(1);
	}

	event_base *event_base = event_base_new();
	evhttp *http_server = evhttp_new(event_base);

	evhttp_bind_socket(http_server, "0.0.0.0", portnum);
	evhttp_set_gencb(http_server, http_cb, &fd); // pass in god object here :)

	event_base_dispatch(event_base);
	evhttp_free(http_server);
	event_base_free(event_base);
	return 0;
}
