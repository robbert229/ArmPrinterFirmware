#include <stdlib.h>
#include <stdio.h>
#include <quicklz.h>
#include <stdint.h>
#include <PrinterConfig.h>


// Swaps MSB LSB
#define ByteSwap32(n) \
    ( ((((unsigned long) n) << 24) & 0xFF000000) |    \
      ((((unsigned long) n) <<  8) & 0x00FF0000) |    \
      ((((unsigned long) n) >>  8) & 0x0000FF00) |    \
      ((((unsigned long) n) >> 24) & 0x000000FF) )

// Reverses the Endian, and the Byte Order, and then write to the specified file
void fwrite32(uint32_t val, FILE *f) {
	uint32_t item = ByteSwap32(val);

	union {
		uint32_t value;
		unsigned char byte[4];
	} input, output;

	input.value = item;
	output.byte[3] = input.byte[0];
	output.byte[2] = input.byte[1];
	output.byte[1] = input.byte[2];
	output.byte[0] = input.byte[3];

	fwrite(&output.value, 4, 1, f);
}

// Writes the step data onto a file. ( Along with Speed ) It also writes using fwrite32 so that the endianness, and byte order get turned into fiq format.
void write_to_fiq(int x, int y, int z, int a, int b, long speed, FILE *fout) {
	long time_buffer;
	long clear_buffer_0;
	long set_buffer_0;
	long clear_buffer_1;
	long set_buffer_1;

	// Sets the buffers to blank.

	time_buffer = speed;
	clear_buffer_0 = 0b00000000000000000000000000000000;
	set_buffer_0 = 0b00000000000000000000000000000000;
	clear_buffer_1 = 0b00000000000000000000000000000000;
	set_buffer_1 = 0b00000000000000000000000000000000;

	// If an axis is not idle, set the set and clear maps to represent this.
	if (x != 0) {
		clear_buffer_0 |= X_STEP_BIT;
		set_buffer_1 |= X_STEP_BIT;
		if (x == STEPPER_NEGATIVE) {
			clear_buffer_0 |= X_DIR_BIT;
			clear_buffer_1 |= X_DIR_BIT;
		} else {
			set_buffer_0 |= X_DIR_BIT;
			set_buffer_1 |= X_DIR_BIT;
		}
	}

	if (y != 0) {
		clear_buffer_0 |= Y_STEP_BIT;
		set_buffer_1 |= Y_STEP_BIT;
		if (y == STEPPER_NEGATIVE) {
			clear_buffer_0 |= Y_DIR_BIT;
			clear_buffer_1 |= Y_DIR_BIT;
		} else {
			set_buffer_0 |= Y_DIR_BIT;
			set_buffer_1 |= Y_DIR_BIT;
		}
	}

	if (z != 0) {
		clear_buffer_0 |= Z_STEP_BIT;
		set_buffer_1 |= Z_STEP_BIT;
		if (z != STEPPER_NEGATIVE) {
			clear_buffer_0 |= Z_DIR_BIT;
			clear_buffer_1 |= Z_DIR_BIT;
		} else {
			set_buffer_0 |= Z_DIR_BIT;
			set_buffer_1 |= Z_DIR_BIT;
		}
	}

	if (a != 0) {
		clear_buffer_0 |= A_STEP_BIT;
		set_buffer_1 |= A_STEP_BIT;
		if (a == STEPPER_NEGATIVE) {
			clear_buffer_0 |= A_DIR_BIT;
			clear_buffer_1 |= A_DIR_BIT;
		} else {
			set_buffer_0 |= A_STEP_BIT;
			set_buffer_1 |= A_DIR_BIT;
		}
	}

	if (b != 0) {
		clear_buffer_0 |= B_STEP_BIT;
		set_buffer_1 |= B_STEP_BIT;
		if (b == STEPPER_NEGATIVE) {
			clear_buffer_0 |= B_DIR_BIT;
			clear_buffer_1 |= B_DIR_BIT;
		} else {
			set_buffer_0 |= B_STEP_BIT;
			set_buffer_1 |= B_DIR_BIT;
		}
	}

	// Writes the data using the function that fixes endianness, and byteorder
	fwrite32(time_buffer, fout);
	fwrite32(clear_buffer_0, fout);
	fwrite32(set_buffer_0, fout);

	fwrite32(time_buffer, fout);
	fwrite32(clear_buffer_1, fout);
	fwrite32(set_buffer_1, fout);
}

// Returns the number of lines in the file. Only use is for counting the percent that the program has parsed.
int fLineCount(FILE *file) {
	int lines = 0;
	char c;
	while ((c = fgetc(file)) != EOF) {
		if (c == '\n') {
			lines++;
		}
	}

	if (fseek(file, 0, SEEK_SET) < 0) {
		return -1;
	} else {
		return lines;
	}
}
