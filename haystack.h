#include <event2/buffer.h>
#import <map>

#define NEEDLE_MAGIC 0xd00fface

struct Key
{
	uint64_t pkey;
	uint32_t skey;

	Key(uint64_t p, uint32_t s) :
	pkey(p), skey(s)
	{

	}

	Key()
	{
		Key(0, 0);
	}

	static void Parse(const char *path, Key &keyout, bool &valid);

	inline bool operator <(const Key &other) const
	{
		printf("key %llx_%x  < %llx_%x\n", pkey, skey, other.pkey, other.skey);
		return pkey < other.pkey || (pkey == other.pkey && skey < other.skey);
	}

	inline bool operator ==(const Key &other) const
	{
		printf("key %llx_%x == %llx_%x\n", pkey, skey, other.pkey, other.skey);
		return pkey == other.pkey && skey == other.skey;
	}

	inline bool operator !=(const Key &other) const
	{
		return !(*this==other);
	}

};

struct NeedleHeader
{
	Key key;
	unsigned char flags;
	size_t size;

	NeedleHeader(const Key k, const unsigned char f, const size_t sz) :
	key(k), flags(f), size(sz)
	{

	}

	NeedleHeader() : key(0, 0), flags(0), size(0)
	{

	}

};

struct MagicHeader
{
	uint32_t magic;
	NeedleHeader header;
};

struct Haystack
{
	int fd;
	std::map<Key, off_t> index;
	bool valid;

	Haystack(const char *path);
	~Haystack();

	off_t OffsetOf(const Key &key);
	int Write(const Key &key, const unsigned char flags, const size_t size, evbuffer *buf);
	int Read(const Key &key, evbuffer *out);

};

