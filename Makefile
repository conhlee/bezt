CC = gcc
CFLAGS = -ggdb -I/opt/homebrew/Cellar/zstd/1.5.7/include -I/opt/homebrew/Cellar/opus/1.5.2/include
LDFLAGS = -L/opt/homebrew/Cellar/zstd/1.5.7/lib -lz -lzstd -L/opt/homebrew/Cellar/opus/1.5.2/lib -lopus
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
