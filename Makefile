
CFLAGS += -DCLS=`getconf LEVEL1_DCACHE_LINESIZE`

memtest: memtest.os
	$(CC) $^ -o $@ $(LDFLAGS)
