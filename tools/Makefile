CC := cc
CFLAGS := -std=c11 -Wall -Wextra -Werror

FONT_OUT := ../kernel/src/console/font_data.c

.PHONY: all generate fuzz clean

all: fontgen fuzzygen

fontgen: fontgen.c
	$(CC) $(CFLAGS) -o $@ $<

fuzzygen: fuzzygen.c fuzzygen.h
	$(CC) $(CFLAGS) -o $@ fuzzygen.c

generate: fontgen
	./fontgen > $(FONT_OUT)

fuzz: fuzzygen
	./fuzzygen $(N)

clean:
	rm -f fontgen fuzzygen
