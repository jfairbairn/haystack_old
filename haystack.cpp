#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/errno.h>
#include <getopt.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <event2/http.h>

#include "haystack.h"

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

bool NeedleHeader::ModifiedSince(const time_t t)
{
	return false;
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
	memset(&header, 0, sizeof(header));

	// scan haystack file; read headers and populate index
	for (off_t offset = 0; offset != -1; offset = lseek(fd, offset + header.header.size + sizeof(header), SEEK_SET))
	{
		if ((! header.Parse(fd)) || header.magic != NEEDLE_MAGIC)
		{
			fprintf(stderr, "Bad bad %x %x\n", header.magic, NEEDLE_MAGIC);
			valid = false;
			return;
		}
		fprintf(stderr, "magic_header_size %d\n", sizeof(header));
		fprintf(stderr, "Read key %lx_%x size %ld: offset %ld into index\n", header.header.key.pkey, header.header.key.skey, header.header.size, offset);
		index[header.header.key] = offset;
	}

}

Haystack::~Haystack()
{
	close(fd);
	valid = false;
}

int Haystack::Write(const Key &key, unsigned char flags, size_t size, const char *content_type, const time_t last_modified, evbuffer *in)
{
	off_t offset = lseek(fd, 0, SEEK_END);
	if (offset == -1)
	{
		return -1;
	}
	
	MagicHeader header;
	NeedleHeader nh(key, flags, size, content_type, last_modified);
	header.magic = NEEDLE_MAGIC;
	header.header = nh;

	if (! header.Serialize(fd)) return -1;
	for (size_t remaining = size; remaining > 0;) {
		int written = evbuffer_write(in, fd);
		if (written == -1)
		{
			return -1;
		}
		remaining -= written;
	}
	if (fsync(fd) == -1) return -1;


	index[nh.key] = offset;

	fprintf(stderr, "Key %lx_%x: offset %ld\n", nh.key.pkey, nh.key.skey, offset);

	return 0;
}

off_t Haystack::OffsetOf(const Key &key) const
{
	auto iter = index.find(key);
	if (iter == index.end()) return -1;
	off_t offset = iter->second;
	return offset;
}

NeedleStatus Haystack::FindNeedle(const Key &key, NeedleData &out) const
{
	off_t offset = OffsetOf(key);
	if (lseek(fd, offset, SEEK_SET) == -1) return NOT_FOUND;
	MagicHeader header;
	if ((! header.Parse(fd)) || header.magic != NEEDLE_MAGIC) return ERROR;
	fprintf(stderr, "Found key %lx_%x size %ld ct %s: offset %ld, data starts at %ld\n", key.pkey, key.skey, header.header.size, header.header.content_type, offset, offset + sizeof(header));

	out.Init(header.header, offset + sizeof(header));
	return FOUND;
}

int Haystack::Read(const NeedleData &needle, evbuffer *out) const
{
	if (out != NULL && evbuffer_add_file(out, dup(fd), needle.offset, needle.header.size) == -1) return -1;
	return needle.header.size;
}
