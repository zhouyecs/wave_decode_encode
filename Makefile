CFLAGS := -Wall -g
TARGETS := wave_decode wave_encode check_version
CLEAN_FILES := mem_mode*.txt mode*.txt wave.fw $(TARGETS)

all: $(TARGETS)

wave_decode: wave_decode_temp_v1.c
	$(CC) $(CFLAGS) $< -o $@

wave_encode: wave_encode_temp_v2.c
	$(CC) $(CFLAGS) $< -o $@

check_version: check_version.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(CLEAN_FILES)

.PHONY: all clean
