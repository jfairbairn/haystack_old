#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdlib.h>

#include "haystack.h"
#include "timeutil.h"

void handle_get(const Key &k, const Haystack &hs, const bool headonly, evhttp_request *request)
{
	evbuffer *buf = evbuffer_new();
	evkeyvalq *output_headers = evhttp_request_get_output_headers(request);
	char last_modified[64];
	memset(last_modified, 0, 64);
	NeedleData needle;
	int needle_status;

	needle_status = hs.FindNeedle(k, needle);
	if (needle_status == NOT_FOUND)
	{
		evhttp_send_error(request, HTTP_NOTFOUND, NULL);
		evbuffer_free(buf);
		return;
	}
	if (needle_status == ERROR)
	{
		evhttp_send_error(request, HTTP_INTERNAL, NULL);
		evbuffer_free(buf);
		return;
	}
	const NeedleHeader &header = needle.header;

	int if_modified_since = rfc822_to_seconds(
		evhttp_find_header(
			evhttp_request_get_input_headers(request), "If-Modified-Since"
		)
	);

	evhttp_add_header(output_headers, "Content-Type",
		strlen(header.content_type) > 0 ? header.content_type : "application/octet-stream"
	);

	seconds_to_rfc822(header.last_modified, last_modified, 63);
	evhttp_add_header(output_headers, "Last-Modified", last_modified);
	evhttp_add_header(output_headers, "Cache-Control", "public, max-age=86400");

	// if (header.last_modified <= if_modified_since)
	// {
	// 	evhttp_send_error(request, HTTP_NOTMODIFIED, NULL);
	// 	evbuffer_free(buf);
	// 	return;
	// }

	if (hs.Read(needle, headonly ? NULL : buf) < 0)
	{
		evhttp_send_error(request, HTTP_INTERNAL, NULL);
		fprintf(stderr, "%s\n", strerror(errno));
		evbuffer_free(buf);
		return;
	}
	evhttp_send_reply(request, HTTP_OK, "OK", buf);
}

void handle_post()
{

}

void http_cb(struct evhttp_request *request, void *data)
{
	Haystack *hs = (Haystack *) data;

	const evhttp_uri *uri = evhttp_request_get_evhttp_uri(request);
	const char *path = evhttp_uri_get_path(uri);

	const char *realpath = path+1;

	Key k;
	bool valid;

	evbuffer *input = evhttp_request_get_input_buffer(request);

	if (strlen(path) < 2 || strchr(realpath, '/') != NULL)
	{
		evhttp_send_error(request, HTTP_NOTFOUND, NULL);
		fprintf(stderr, "Invalid path %s\n", path);
		return;
	}

	Key::Parse(realpath, k, valid);
	if (!valid)
	{
		evhttp_send_error(request, HTTP_NOTFOUND, NULL);
		fprintf(stderr, "Invalid key %s\n", realpath);
		return;
	}
	int ret;
	const char *content_type;

	bool headonly = false;

	switch (evhttp_request_get_command(request))
	{	
		case EVHTTP_REQ_POST:
		case EVHTTP_REQ_PUT:

			content_type = evhttp_find_header(
				evhttp_request_get_input_headers(request), "Content-Type");

			if (content_type == NULL)
			{
				content_type = "";
			}

			if (hs->Write(k, 0, evbuffer_get_length(input), content_type, now(), input) < 0)
			{
				evhttp_send_error(request, HTTP_BADREQUEST, NULL);
				fprintf(stderr, "%s\n", strerror(errno));
				break;
			}
			evhttp_send_error(request, HTTP_OK, NULL);
			break;

		case EVHTTP_REQ_HEAD:
			headonly = true;
		case EVHTTP_REQ_GET:
			handle_get(k, *hs, headonly, request);
			break;

		default:
		// shouldn't get here -- allowed request types are set elsewhere
		// (see calls to evhttp_set_allowed_methods())
			evhttp_send_error(request, HTTP_BADMETHOD, NULL);

	}

}

typedef int (*mainfunc)(int, char * const *);

#define MAIN server

int server(int argc, char * const *argv)
{
	static struct option long_options[] =
	{
		{"port", required_argument, 0, 'p'},
		{"file", required_argument, 0, 'f'}
	};

	int option_index = 0;
	int c;
	int portnum = -1;
	int fd = -2;
	char path[256];
	path[0]=0;
	while (1) {

		c = getopt_long(argc, argv, "p:f:", long_options, &option_index);
		if (c == -1) break;

		switch (c)
		{
			case 'f':
				strncpy(path, optarg, 256);
				break;
			case 'p':
				portnum = (int) strtol(optarg, NULL, 10);
				break;
		}

	}

	if (portnum < 0 || strlen(path)==0) {
		fprintf(stderr, "Couldnt start: portnum=%d, path=\"%s\"\n", portnum, path);
		exit(1);
	}
	fprintf(stderr, "Start: portnum=%d, path=\"%s\"\n", portnum, path);

	event_base *event_base = event_base_new();
	evhttp *http_server = evhttp_new(event_base);

	Haystack h(path);

	if (evhttp_bind_socket(http_server, "0.0.0.0", portnum) == -1)
	{
		fprintf(stderr, "Bind fail\n");
		exit(1);
	}
	evhttp_set_allowed_methods(http_server, EVHTTP_REQ_GET|EVHTTP_REQ_PUT|EVHTTP_REQ_POST|EVHTTP_REQ_HEAD);
	evhttp_set_gencb(http_server, http_cb, &h);

	event_base_dispatch(event_base);
	evhttp_free(http_server);
	event_base_free(event_base);
	return 0;
};

int main(int argc, char * const *argv) {
	return MAIN(argc, argv);
}
