CC = gcc
CFLAGS += $(shell pkg-config --cflags zlib zstd opus)
LDFLAGS += $(shell pkg-config --libs zlib zstd opus)
TARGET = bezt
SOURCES = main.c files.c compression.c list.c common.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = files.h compression.h list.h common.h beaProcess.h bfsarProcess.h luaProcess.h opusProcess.h

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
