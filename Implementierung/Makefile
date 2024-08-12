program_files := main.c modules/util.c modules/brightness_contrast.c modules/brightness_contrast_sse.c

.PHONY: all
all:
	gcc -o main.out $(program_files) -std=c17 -O3 -g -Wall -Wextra -Wpedantic -msse4.2

.PHONY: clean
clean:
	rm -f main.out create_ppm_image.out

