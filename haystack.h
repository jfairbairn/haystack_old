#include <event2/buffer.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#import <sparsehash/sparse_hash_map>

#include "haystack.pb.h"

#define NEEDLE_MAGIC 0xd00fface

using google::sparse_hash_map;

template <class T> class PB
{
protected:
	virtual void Init(const T &t) = 0;
	virtual void ToPB(T &t) const = 0;

public:
	virtual ~PB()
	{

	}

	const bool Parse(const int fd)
	{
		T t;

		t.ParseFromFileDescriptor(fd);
		if (! t.IsInitialized())
		{
			fprintf(stderr, "parse fail: Missing fields: %s\n---\n", t.InitializationErrorString().c_str());
			return false;
		}

		Init(t);
		return true;
	}

	const bool Serialize(const int fd)
	{
		T t;
		ToPB(t);
		if (! t.IsInitialized())
		{
			fprintf(stderr, "Missing fields: %s\n", t.InitializationErrorString().c_str());
			return false;
		}
		return t.SerializeToFileDescriptor(fd);
	}

};

struct Key : public PB<haystack::pb::Key>
{
	uint64_t pkey;
	uint32_t skey;

	Key(uint64_t p, uint32_t s) :
	pkey(p), skey(s)
	{

	}

	Key(const haystack::pb::Key &pbk)
	{
		Init(pbk);
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

	void Init(const haystack::pb::Key &pbk)
	{
		pkey = pbk.pkey();
		skey = pbk.skey();
	}

	void ToPB(haystack::pb::Key &pbk) const
	{
		pbk.set_pkey(pkey);
		pbk.set_skey(skey);
	}

};

struct NeedleHeader : public PB<haystack::pb::NeedleHeader>
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

	void Init(const haystack::pb::NeedleHeader &pbnh)
	{
		key.Init(pbnh.key());
		flags = pbnh.flags();
		size = pbnh.size();
		last_modified = pbnh.last_modified();
		memset(content_type, 0, 64);
		strncpy(content_type, pbnh.content_type().c_str(), 63);
	}

	void ToPB(haystack::pb::NeedleHeader &pbnh) const
	{
		haystack::pb::Key *pbk = new haystack::pb::Key();
		key.ToPB(*pbk);
		pbnh.set_allocated_key(pbk);
		pbnh.set_flags(flags);
		pbnh.set_size(size);
		pbnh.set_content_type(content_type, strlen(content_type));
		pbnh.set_last_modified(last_modified);
	}

	bool ModifiedSince(const time_t t);
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

struct MagicHeader : public PB<haystack::pb::MagicHeader>
{
	uint32_t magic;
	NeedleHeader header;
	int magic_header_size;

	MagicHeader() :
	magic(NEEDLE_MAGIC)
	{

	}

	void Init(const haystack::pb::MagicHeader &pbmh)
	{
		magic = pbmh.magic();
		header.Init(pbmh.header());
		magic_header_size = pbmh.ByteSize();
	}

	void ToPB(haystack::pb::MagicHeader &pbmh) const
	{
		pbmh.set_magic(magic);
		haystack::pb::NeedleHeader *pbnh = new haystack::pb::NeedleHeader();
		header.ToPB(*pbnh);
		pbmh.set_allocated_header(pbnh);
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
