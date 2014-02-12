int readfull(const int fd, void *dstv, const size_t len)
{
	for (size_t to_read = len; to_read > 0;)
	{
		int ret = read(fd, dstv, to_read);
		if (ret == -1) return -1;
		if (ret == 0) return 0;
		to_read -= ret;
	}
	return len;
}

int writefull(const int fd, const void *srcv, const size_t len)
{
	for (size_t to_write = len; to_write > 0;)
	{
		int ret = write(fd, srcv, to_write);
		if (ret == -1) return -1;
		to_write -= ret;
	}
	return len;
}

