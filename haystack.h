#include <event2/buffer.h>

#include <sparsehash/sparse_hash_map>

#define NEEDLE_MAGIC 0xd00fface

using google::sparse_hash_map;

template <class T> int readfull(int fd, T *t)
{
	void *buf = (void *)t;

	for (size_t remaining = sizeof(T); remaining > 0;)
	{
		int nread = read(fd, buf, remaining);
		if (nread <= 0)
		{
			return nread;
		}
		remaining -= nread;
	}
	return sizeof(T);
}

template <class T> int writefull(int fd, T *t)
{
	void *buf = (void *)t;
	for (size_t remaining = sizeof(T); remaining > 0;)
	{
		int nwritten = write(fd, buf, remaining);
		if (nwritten <= 0)
		{
			return nwritten;
		}
		remaining -= nwritten;
	}
	return sizeof(T);
}

template <class T> class Serializable
{
protected:

public:
	virtual ~Serializable()
	{

	}

	const bool Parse(const int fd)
	{
		return readfull(fd, this) > 0;
	}

	const bool Serialize(const int fd)
	{
		return writefull(fd, (T*)this) > 0;
	}

};

struct Key : public Serializable<Key>
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
		return pkey < other.pkey || (pkey == other.pkey && skey < other.skey);
	}

	inline bool operator ==(const Key &other) const
	{
		return pkey == other.pkey && skey == other.skey;
	}

	inline bool operator !=(const Key &other) const
	{
		return !(*this==other);
	}

	void Init(const Key &key)
	{

	}

	void ToPB(Key &key) const
	{

	}
};

struct NeedleHeader : public Serializable<NeedleHeader>
{
	Key key;
	unsigned char flags;
	size_t size;
	char content_type[64];
	time_t last_modified;

	NeedleHeader(const Key k, const unsigned char f, const size_t sz, const char *_content_type, const time_t _last_modified) :
	key(k), flags(f), size(sz), last_modified(_last_modified)
	{
		memset(content_type, 0, 64);
		strncpy(content_type, _content_type, 63);
	}

	NeedleHeader() : key(0, 0), flags(0), size(0), last_modified(0)
	{
		memset(content_type, 0, 64);
	}

	void Init(const NeedleHeader &nh)
	{
		key = nh.key;
		flags = nh.flags;
		size = nh.size;
		memcpy(content_type, nh.content_type, sizeof(content_type));
		last_modified = nh.last_modified;
	}

	bool ModifiedSince(const time_t t);

	void ToPB(NeedleHeader &nh) const
	{

	}

};

enum NeedleStatus
{
	FOUND,
	NOT_FOUND,
	ERROR
};

struct NeedleData
{
	NeedleHeader header;
	off_t offset;

	NeedleData()
	{
		memset(this, 0, sizeof(NeedleData));
	}

	void Init(const NeedleHeader &needle_header, const off_t _offset)
	{
		header = needle_header;
		offset = _offset;
	}
};

struct MagicHeader : public Serializable<MagicHeader>
{
	uint32_t magic;
	NeedleHeader header;

	MagicHeader() :
	magic(NEEDLE_MAGIC)
	{

	}

	virtual void Init(const MagicHeader &mh)
	{

	}

	virtual void ToPB(MagicHeader &mh) const
	{

	}

};

struct KeyHash
{
	size_t operator()(const Key & key) const
	{
		return key.pkey;
	}
};

struct Haystack
{
	int fd;
	sparse_hash_map<Key, off_t, KeyHash> index;
	bool valid;

	Haystack(const char *path);
	~Haystack();

	off_t OffsetOf(const Key &key) const;

	NeedleStatus FindNeedle(const Key &key, NeedleData &out) const;

	int Write(
		const Key &key,
		const unsigned char flags,
		const size_t size,
		const char *content_type,
		const time_t last_modified,
		evbuffer *buf
	);

	int Read(const NeedleData &needle, evbuffer *out) const;

};
