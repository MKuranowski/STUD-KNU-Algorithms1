CFLAGS := -g
CFLAGS_REAL := -W -Wall -Wextra -Werror $(CFLAGS)
LDFLAGS := -lm

.PHONY: all clean
all: hw5_1 hw5_2
clean:
	rm -f hw5_1 hw5_2

hw5_1: hw5.c
	$(CC) $(CFLAGS) -DPART=1 -o $@ $^ $(LDFLAGS)

hw5_2: hw5.c
	$(CC) $(CFLAGS) -DPART=2 -o $@ $^ $(LDFLAGS)
