/*
 ARMStrongPrinter is a nascent and native 3d printer firmware that is designed to work with ARM SOMs.
 Copyright (C) 2013  John Rowley (johnrowleyster@gmail.com)

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Revision 0.1
 Making a focus to be able to handle everything without the need to babysit the program.
 In the past I was only able to load 12MB chunks of fiq data, and nothing would automatically
 happen. I had to turn the temps on seperately from loading the data, and this program is an
 attempt to fiq that.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <ArmStrongClient.h>
#include <quicklz.h>

bool verbose = false;
bool compressed = false;
bool print = true;
char* inputFile;
char* descriptorFile;
FILE* fin, fout;
int main(int argc, char *argv[]) {
	opterr = 0;
	int c;
	while ((c = getopt(argc, argv, "cvi:")) != -1) {
		switch (c) {
		case 'c':
			compressed = true;
			break;
		case 'v':
			verbose = true;
			break;
		case 'i':
			if (access(optarg, F_OK) != -1) {
				inputFile = optarg;
				break;
			} else {
				printf("Source File Doesn't Exit: %s\nAborting\n", optarg);
				exit(-2);
			}
		case 'n':
			print = false;
			break;
		case 's':
			if (access(optarg, F_OK) != -1) {
				descriptorFile = optarg;
				break;
			} else {
				printf("Source File Doesn't Exist: %s\nAborting\n", optarg);
				break;
			}

		}
	}

	if (optind < argc || argc == 1) {
		printf("\n%s: invalid usage\n", argv[0]);
		printf(
				"\nusage: %s [-i INPUT -v -c -n -s DESCRIPTOR]\nprints the fcode\n",
				argv[0]);
		printf("\t\t-i INPUT\t\tthe file to read from\n");
		printf("\t\t-v \t\t\tdisplay extra information (be verbose)\n");
		printf("\t\t-n \t\t\tdon't print the program\n");
		printf("\t\t-s \t\t\tthe file to save the descriptor to\n");
		printf(
				"\t\t-c \t\t\tthe input is compressed, and needs to be decompressed\n\n\n");
		exit(-4);
	}

}
