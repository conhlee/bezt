PKGCONFIG_ZSTD ?= 0

CC = gcc
CFLAGS += $(shell pkg-config --cflags zlib opus)
LDFLAGS += $(shell pkg-config --libs zlib opus)
TARGET = bezt
SOURCES = main.c files.c compression.c list.c common.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = files.h compression.h list.h common.h beaProcess.h bfsarProcess.h luaProcess.h opusProcess.h


ifeq (PKGCONFIG_ZSTD,0)
CFLAGS += $(shell pkg-config --cflags zstd)
LDFLAGS += $(shell pkg-config --libs zstd)
else
LDFLAGS += -lzstd
endif

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
