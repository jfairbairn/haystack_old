#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/errno.h>
#include <getopt.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#include "haystack.h"
#include "fileutil.h"

void Key::Parse(const char *path, Key &keyout, bool &valid)
{
	size_t len = strnlen(path, 26);
	if (len != 25)
	{
		valid = false;
		fprintf(stderr, "Invalid path length %zu for %s\n", len, path);
		return;
	}
	char buf[9];

	memset(buf, 0, 9);
	strncpy(buf, path, 8);
	uint64_t _pkey = strtol(buf, NULL, 16) << 32;

	memset(buf, 0, 9);
	strncpy(buf, path+8, 8);
	_pkey += strtol(buf, NULL, 16);

	keyout.pkey = _pkey;

	memset(buf, 0, 9);
	strncpy(buf, path+17, 8);
	uint32_t _skey = strtol(buf, NULL, 16);
	keyout.skey = _skey;

	valid = true;
}

Haystack::Haystack(const char *path) : index(), valid(true)
{
	fd = open(path, O_CREAT|O_RDWR|O_APPEND, 0644);
	if (fd == -1)
	{
		valid = false;
		return;
	}
	MagicHeader header;

	// scan haystack file; read headers and populate index
	for (off_t offset = 0; offset != -1; offset = lseek(fd, header.header.size, SEEK_CUR))
	{
		size_t nread = readfull(fd, &header, sizeof(header));
		if (nread == -1 || header.magic != NEEDLE_MAGIC)
		{
			valid = false;
			return;
		}
		if (nread == 0)
		{
			return;
		}
		fprintf(stderr, "Read key %llx_%x: offset %lld into index\n", header.header.key.pkey, header.header.key.skey, offset);
		index.erase(header.header.key);
		index.insert(std::pair<Key, off_t>(header.header.key, offset));
	}

}

Haystack::~Haystack()
{
	close(fd);
	valid = false;
}

int Haystack::Write(const Key &key, unsigned char flags, size_t size, const char *content_type, evbuffer *in)
{
	off_t offset = lseek(fd, 0, SEEK_END);
	if (offset == -1)
	{
		return -1;
	}
	
	MagicHeader header;
	NeedleHeader nh(key, flags, size, content_type);
	header.magic = NEEDLE_MAGIC;
	header.header = nh;

	if (writefull(fd, &header, sizeof(header)) == -1)
	{
		return -1;
	}
	for (size_t remaining = size; remaining > 0;) {
		int written = evbuffer_write(in, fd);
		if (written == -1)
		{
			return -1;
		}
		remaining -= written;
	}
	if (fsync(fd) == -1) return -1;


	index.erase(nh.key);
	index.insert(std::pair<Key, off_t>(nh.key, offset));

	fprintf(stderr, "Key %llx_%x: offset %lld\n", nh.key.pkey, nh.key.skey, offset);

	return 0;
}

off_t Haystack::OffsetOf(const Key &key)
{
	auto iter = index.find(key);
	if (iter == index.end()) return -1;
	off_t offset = iter->second;
	return offset;
}

int Haystack::Read(const Key &key, const char *&content_type, evbuffer *out)
{
	off_t offset = OffsetOf(key);

	if (lseek(fd, offset, SEEK_SET) == -1) return -2;

	MagicHeader header;
	ssize_t nread = readfull(fd, &header, sizeof(header));
	content_type = header.header.content_type;
	if (header.magic != NEEDLE_MAGIC) return -4;
	if (header.header.key != key) return -5;

	if (out != NULL && evbuffer_add_file(out, dup(fd), offset+sizeof(header), header.header.size) == -1) return -6;

	return header.header.size;
}
