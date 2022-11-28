CFLAGS := -g
CFLAGS_REAL := -W -Wall -Wextra -Werror $(CFLAGS)
LDFLAGS := -lm

hw5_2: hw5_common.c hw5_oneway_dijkstra.c hw5_2.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: all clean
all: hw5_2
clean:
	rm hw5_2
