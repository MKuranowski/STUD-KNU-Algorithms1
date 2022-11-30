CFLAGS := -g
CFLAGS_REAL := -W -Wall -Wextra -Werror $(CFLAGS)
LDFLAGS := -lm

.PHONY: all clean
all: hw5_1 hw5_2
clean:
	rm -f hw5_1 hw5_2

hw5_1: hw5_common.c hw5_1and2.c
	$(CC) $(CFLAGS) -DMAX_LEN=8 -o $@ $^ $(LDFLAGS)

hw5_2: hw5_common.c hw5_1and2.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
