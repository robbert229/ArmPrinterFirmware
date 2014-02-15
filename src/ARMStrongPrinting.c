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

/*
 This program can be run on the printer, or on a desktop host. 
 It takes the gcode and parses it into fiq code; the code that is then
 executed on the printer via a FIQ. All the fiq code is is a 32 bit long
 bit flag that represents the states of all of the pins on a bus. This is a 
 ineficient way of doing things but it allows Linux SOMs to ignore linux,
 and get high performance. 

 The program can be broken up into several main section. The first key section
 is the gcode parsing. I parse only the movement related gcode, there is a 
 suprising amount of code that is dedicated to working with controlling the
 hot bed, extruder temp, etc. The gcode is in mm, but this program makes all
 of its operations based on steps, so the next thing I do is convert the 
 measurements I got from gcode into steps. I then pass it all off into the 
 stepping section.

 the stepping section at its' core is a breshenham algorithm that records which
 direction it moves at every tick. It is axis independant and knows no difference
 between any of the axes. This combined with the algorithm being flexible allows
 multiple extruders right out of the box. Infact currently the algorithm is set 
 for 5 steppers (two extruders). Each time it steps it sends that data out to the 
 fiq write section.

 The fiq write section is what writes the data into fiq format. The directions that
 each axis are going to step are passed in as integers. I then take a 32bit long and
 use it as a bitflag. (The information on what bits do what are included in the header)
 In my case I had to reverse the endianness and the byte order as well. I then write
 this information into a file.

 If the program was told to compress it will now compress the fcode into 
 a significantly smaller file. I have an example where I reduced 180 MB of
 fiq code into 6 MB. I use quicklz to handle the compression.

 Now I am ready to pass all of the information off to my printer. Now it is
 ARMStrongFiq's turn. It is the second program. It will take in  
 cfiq (compressed fiq data), or standard fiq files. It also holds a settings file
 which can be used to set the temperatures of the hotbed, extruder, etc. It is 
 currently not written at the moment. 

 Now it is time to acknowledge those who have helped me with all of this.
 Brian Lilly is responsible for writing the temperature controll, along with all
 of the initialization for the steppers. Brent Crosby supplied me with the printer
 and gave me the opportunity to write this program in the first place.
 */

/* Revision 0.1 
 The main focus of the program is to interpret g0 and g1, both of which are 
 the core printing functions. The program starts by opening test.gcode. 
 This file contains gcode to print. I have designed my interpretation of 
 gcode based on a minimal if not insufficient version of the reprap styled gcode.
 From here it proceeds to parse the file line by line and execute the instructions. 
 */

/* Revision 0.2
 At this point I have added in g92, and added in the z,a,and b axes.
 It now uses integer based calculations as well 
 */

/* Revision 0.3
 I am now changing the way I write fiq data, the new method is much cleaner and effecient.
 This version is the first version to actually work. 
 */

/* Revision 0.4
 I have decoupled some of the key parts as to allow different utilities access 
 to the needed functions. I also cleaned up the solution by getting rid of old files.
 */

/* Revision 0.5 
 Merged all files back into one, preparing for the first public release. 
 Adding in file compression so that upload times can be reduced 

 Also working on a seperate tool (ArmStrongFiq) to load the fiq files intelligently 
 without constant supervision. This tool will also handle the controlling the hotend
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>
#include "ArmStrongPrinting.h"
#include "quicklz.h"

#define COMMANDBUFLEN 128

FILE *fin, *fout; // Input and Output Files
double x_pos, y_pos, z_pos, a_pos, b_pos; // Positions
int lineNumber; // The line number inside the Input file
int totalLineNumber; // The total number of lines that the input file has
char *sourceFileName = NULL; // The name of the input file
char *destinationFileName = NULL; // name of the output file
bool isVerbose = false; // is verbose
bool isCompressing = false; // whether or not it compresses the fiq data after it writes it.

int main(int argc, char *argv[]) {
	opterr = 0;
	int c;
	while ((c = getopt(argc, argv, "cvo:i:")) != -1) {
		switch (c) {

		case 'c':
			isCompressing = true;
			break;
		case 'v':
			isVerbose = true;
			break;

		case 'o':
			destinationFileName = optarg;
			break;

		case 'i':
			if (access(optarg, F_OK) != -1) {
				sourceFileName = optarg;
				break;
			} else {
				printf("Source File Doesn't Exits: %s\nAborting\n", optarg);
				exit(-2);
			}
		}
	}

	if (optind < argc || argc == 1) {
		printf("\n%s: invalid usage\n", argv[0]);
		printf(
				"\nusage: %s [-i INPUT -o OUTPUT -v -c]\n\nparses gcode into fcode or cfcode\n\n",
				argv[0]);
		printf("\t\t-o OUTPUT\t\tthe file to write to\n");
		printf("\t\t-i INPUT\t\tthe file to read from\n");
		printf("\t\t-v \t\t\tdisplay extra information (be verbose)\n");
		printf("\t\t-c \t\t\toutput cfcode instead of fcode\n\n\n");
		exit(-4);
	}

	if (sourceFileName == NULL || destinationFileName == NULL) {
		printf("Not Enough Arguments\n");
		exit(-1);
	}

	char rawInput[COMMANDBUFLEN];
	// Open gcode
	fin = fopen(sourceFileName, "r");
	if (!fin) {
		printf("Can't Open %s\n", sourceFileName);
		return -1;
	}

	// If you are compressing then we will write to a temp file for now. Other wise we will write to the specified file.
	if (!isCompressing) {
		fout = fopen(destinationFileName, "wb");
		if (!fout) {
			printf("Can't Open %s\n", destinationFileName);
			return -1;
		}
	} else {
		fout = fopen("temp.fcode", "wb");
		if (!fout) {
			printf("Can't Open a temporary file:\ttemp.fcode\n");
			return -1;
		}
	}

	totalLineNumber = fLineCount(fin);

	while (fgets(rawInput, COMMANDBUFLEN, fin) != NULL) {

		// Prints Overall Progress
		static int printIter = 1;
		if (lineNumber >= (printIter / 10.0) * totalLineNumber) {
			if (isVerbose) {
				printf("%i%% Percent Done\n", 10 * printIter);
			}
			printIter++;
		}
		lineNumber++;

		// Searches for a comment, if one exists, terminate the string at the comment
		char *line = strstr(rawInput, ";");
		if (line != NULL) {
			*line = 0;
		}
		if (strstr(rawInput, "\n")) {
			*strstr(rawInput, "\n") = 0;
		}

		// Parses each individual gcode
		if (strstr(rawInput, "G0 ") != NULL) {
			parseG0(rawInput, lineNumber);
		} else if (strstr(rawInput, "G1 ") != NULL) {
			parseG1(rawInput, lineNumber);
		} else if (strstr(rawInput, "G28 ") != NULL) {
			parseG28(rawInput, lineNumber);
		} else if (strstr(rawInput, "G92 ") != NULL) {
			parseG92(rawInput, lineNumber);
		} else if (strcmp("", rawInput) == 0) {
			// Do Nothing, this line is a comment
		} else {
			// An unknown gcode, ignore it, there are a lot of fluff gcodes that are for setting things like temperature.
			printf("[%i] Unknown gcode function [%s]\n", lineNumber, rawInput);
		}
	}

	if (isVerbose) {
		printf("100%% Percent Done\n");
	}

	fclose(fin);
	fclose(fout);

	if (isCompressing) {
		if (isVerbose) {
			printf("Now Starting Compression\n");
		}

		fin = fopen("temp.fcode", "rb");
		fout = fopen(destinationFileName, "wb");
		unsigned long saved = compressFile(fin, fout);

		if (isVerbose) {
			printf("Saved %li Bytes\n", saved);
		}

		fclose(fin);
		fclose(fout);
		system("del temp.fcode");
	}

	return 1;
}

/* G0 is for moving around quickly, usually not for printing, but rather repositioning. */
void parseG0(char *input, int lineNumber) {
	/* At the moment there is no difference between G0 and G1, this is true for most 3d printers. */
	parseG1(input, lineNumber);
}

/* G1 is the standard code for manuvering while extruding and is the most common command. */
void parseG1(char *input, int lineNumber) {
	// stepper_rate is the Micro Seconds Per Step delay. It is the delay that is put into the fiq.
	static unsigned long stepper_rate = 0;
	double xTargetMilimeter = x_pos; /* get xAxis from input. */
	double yTargetMilimeter = y_pos; /* get yAxis from input. */
	double zTargetMilimeter = z_pos; /* get zAxis from input. */
	double aTargetMilimeter = a_pos; /* get aAxis from input. */
	double bTargetMilimeter = b_pos; /* get bAxis from input. */

	// Check to see if any specific axis are being set
	char *xPlace = strstr(input, "X");
	if (xPlace == NULL) {
		xTargetMilimeter = x_pos;
	} else {
		char *start = xPlace + 1;
		char floatHolder[12];
		memset(floatHolder, 0, 12);

		while (*(xPlace++) != ' ' && *xPlace != 0) {
		}
		strncpy(floatHolder, start, xPlace - start);
		xTargetMilimeter = atof(floatHolder);
	}

	char *yPlace = strstr(input, "Y");
	if (yPlace == NULL) {
		yTargetMilimeter = y_pos;
	} else {
		char *start = yPlace + 1;
		char floatHolder[12];
		memset(floatHolder, 0, 12);

		while (*(yPlace++) != ' ' && *yPlace != 0) {
		}
		strncpy(floatHolder, start, yPlace - start);
		yTargetMilimeter = atof(floatHolder);
	}

	char *zPlace = strstr(input, "Z");
	if (zPlace == NULL) {
		zTargetMilimeter = z_pos;
	} else {
		char *start = zPlace + 1;
		char floatHolder[12];
		memset(floatHolder, 0, 12);

		while (*(zPlace++) != ' ' && *zPlace != 0) {
		}
		strncpy(floatHolder, start, zPlace - start);
		zTargetMilimeter = atof(floatHolder);
	}

	char *aPlace = strstr(input, "E");
	if (aPlace == NULL) {
		aTargetMilimeter = a_pos;
	} else {
		char *start = aPlace + 1;
		char floatHolder[12];
		memset(floatHolder, 0, 12);
		while (*(aPlace++) != ' ' && *aPlace != 0) {
		}
		strncpy(floatHolder, start, aPlace - start);
		aTargetMilimeter = atof(floatHolder);
	}

	char *bPlace = strstr(input, "B");
	if (bPlace == NULL) {
		bTargetMilimeter = b_pos;
	} else {
		char *start = bPlace + 1;
		char floatHolder[12];
		memset(floatHolder, 0, 12);
		while (*(bPlace++) != ' ' && *bPlace != 0) {
		}
		strncpy(floatHolder, start, bPlace - start);
		bTargetMilimeter = atof(floatHolder);
	}

	// the input is in milimeters per minute, and the output needs to be in milisecond per step

	int xTargetSteps = xTargetMilimeter * XSTEPSPERMILIMETER;
	int yTargetSteps = yTargetMilimeter * YSTEPSPERMILIMETER;
	int zTargetSteps = zTargetMilimeter * ZSTEPSPERMILIMETER;
	int aTargetSteps = aTargetMilimeter * ASTEPSPERMILIMETER;
	int bTargetSteps = bTargetMilimeter * BSTEPSPERMILIMETER;

	int xCurrentSteps = x_pos * XSTEPSPERMILIMETER;
	int yCurrentSteps = y_pos * YSTEPSPERMILIMETER;
	int zCurrentSteps = z_pos * ZSTEPSPERMILIMETER;
	int aCurrentSteps = a_pos * ASTEPSPERMILIMETER;
	int bCurrentSteps = b_pos * BSTEPSPERMILIMETER;

	// check to see if a new speed is being set
	char *speedPlace = strstr(input, "F");
	if (speedPlace != NULL) {
		char *start = speedPlace + 1;
		char floatHolder[16];
		memset(floatHolder, 0, 16);
		while (*(speedPlace++) != ' ' && *speedPlace != 0) {
		}
		strncpy(floatHolder, start, speedPlace - start);

		// speed in mm per minute
		double maximum_speed = atof(floatHolder);

		/*int a_distance_s = (aTargetSteps - aCurrentSteps);
		double a_distance_m = a_distance_s / ASTEPSPERMILIMETER;*/

		/*int f = (int) (60000 / (maximum_speed * ASTEPSPERMILIMETER) * 1000);
		printf("Rate: %i\n", f);*/
		
		/*
		double f = maximum_speed;
		int qs[5] = {xCurrentSteps,yCurrentSteps,zCurrentSteps,aCurrentSteps,bCurrentSteps};
		int ps[5] = {xTargetSteps,yTargetSteps,zTargetSteps,aTargetSteps,bTargetSteps};
		double zs = fiveDimensionalDistanceInt(qs,ps);
		*/
		double qd[5] = {
			xCurrentSteps / XSTEPSPERMILIMETER,
			yCurrentSteps / YSTEPSPERMILIMETER,
			zCurrentSteps / ZSTEPSPERMILIMETER,
			aCurrentSteps / ASTEPSPERMILIMETER,
			bCurrentSteps / BSTEPSPERMILIMETER
		};

		double pd[5] = {
			xTargetMilimeter,
			yTargetMilimeter,
			zTargetMilimeter,
			aTargetMilimeter,
			bTargetMilimeter
		};

		double z = fiveDimensionalDistanceDouble(qd,pd);
		double a = maximum_speed / 60000.0;
		double b = z;
		double c = b / a;
		double d = c / b;
		//printf("A: %f mm/min, B: %fmm, C:%f mili, D: %fmili\n",a,b,c,d);
		//printf("A: %f mm/min, B: %fmm, C:%f mili, D: %imili\n",a,b,c,(int)d);
		if(d < 150) d = 150;
		stepper_rate = (unsigned long)d * 2;
	}

	// Call bresenham, and yes I know I spelled it incorrectly.
	bresenham(xCurrentSteps, yCurrentSteps, zCurrentSteps, aCurrentSteps,
			bCurrentSteps, xTargetSteps, yTargetSteps, zTargetSteps,
			aTargetSteps, bTargetSteps, stepper_rate, fout);

	x_pos = xTargetMilimeter;
	y_pos = yTargetMilimeter;
	z_pos = zTargetMilimeter;
	a_pos = aTargetMilimeter;
	b_pos = bTargetMilimeter;
}

// Sets the current position for the specified axes, if no axes or values
// are specified then the current position is considered 0,0,0
void parseG92(char *input, int lineNumber) {
	double newX = x_pos;
	double newY = y_pos;
	double newZ = z_pos;
	double newA = a_pos;
	double newB = b_pos;

	// Check to see if any of the axis are being set
	char *xPlace = strstr(input, "X");
	if (xPlace != NULL) {
		char *start = xPlace + 1;
		char floatHolder[12];
		memset(floatHolder, 0, 12);
		while (*(xPlace++) != ' ' && xPlace != 0) {
		}
		strncpy(floatHolder, start, xPlace - start);
		newX = atof(floatHolder);
	}

	char *yPlace = strstr(input, "Y");
	if (yPlace != NULL) {
		char *start = yPlace + 1;
		char floatHolder[12];
		memset(floatHolder, 0, 12);
		while (*(yPlace++) != ' ' && yPlace != 0) {
		}
		strncpy(floatHolder, start, yPlace - start);
		newY = atof(floatHolder);
	}

	char *zPlace = strstr(input, "Z");
	if (zPlace != NULL) {
		char *start = zPlace + 1;
		char floatHolder[12];
		memset(floatHolder, 0, 12);
		while (*(zPlace++) != ' ' && zPlace != 0) {
		}
		strncpy(floatHolder, start, zPlace - start);
		newZ = atof(floatHolder);
	}

	char *aPlace = strstr(input, "E");
	if (aPlace != NULL) {
		char *start = aPlace + 1;
		char floatHolder[12];
		memset(floatHolder, 0, 12);
		while (*(aPlace++) != ' ' && aPlace != 0) {
		}
		strncpy(floatHolder, start, aPlace - start);
		newA = atof(floatHolder);
	}

	char *bPlace = strstr(input, "B");
	if (bPlace != NULL) {
		char *start = bPlace + 1;
		char floatHolder[12];
		memset(floatHolder, 0, 12);
		while (*(bPlace++) != ' ' && bPlace != 0) {
		}
		strncpy(floatHolder, start, bPlace - start);
		newB = atof(floatHolder);
	}

	// Set the axis to the new settings.
	x_pos = newX;
	y_pos = newY;
	z_pos = newZ;
	a_pos = newA;
	b_pos = newB;
}

/* Moves the specified axis to the origin. If no axes are specified then all of the axes are centered. */
void parseG28(char *input, int lineNumber) {
	double xTargetMilimeter = x_pos;
	double yTargetMilimeter = y_pos;
	double zTargetMilimeter = z_pos;
	double aTargetMilimeter = a_pos;
	double bTargetMilimeter = b_pos;

	// If an axis is in this string it means that the specific axis needs to be centered.

	if (strstr(input, "X") != NULL) {
		xTargetMilimeter = 0;
	}

	if (strstr(input, "Y") != NULL) {
		yTargetMilimeter = 0;
	}

	if (strstr(input, "Z") != NULL) {
		zTargetMilimeter = 0;
	}

	if (strstr(input, "A") != NULL) {
		aTargetMilimeter = 0;
	}

	if (strstr(input, "B") != NULL) {
		bTargetMilimeter = 0;
	}

	// Converts gcode format (Milimeters) to my format (Steps)

	int xTargetSteps = xTargetMilimeter * XSTEPSPERMILIMETER;
	int yTargetSteps = yTargetMilimeter * YSTEPSPERMILIMETER;
	int zTargetSteps = zTargetMilimeter * ZSTEPSPERMILIMETER;
	int aTargetSteps = aTargetMilimeter * ASTEPSPERMILIMETER;
	int bTargetSteps = bTargetMilimeter * BSTEPSPERMILIMETER;

	int xCurrentSteps = x_pos * XSTEPSPERMILIMETER;
	int yCurrentSteps = y_pos * YSTEPSPERMILIMETER;
	int zCurrentSteps = z_pos * ZSTEPSPERMILIMETER;
	int aCurrentSteps = a_pos * ASTEPSPERMILIMETER;
	int bCurrentSteps = b_pos * BSTEPSPERMILIMETER;

	// Calls the incorrectly named algorithm

	bresenham(xCurrentSteps, yCurrentSteps, zCurrentSteps, aCurrentSteps,
			bCurrentSteps, xTargetSteps, yTargetSteps, zTargetSteps,
			aTargetSteps, bTargetSteps, 0x000000F0, fout);
}

// The incorrectly named algorithm that approximates a line on 5 dimensions. There are no differences between axis. This allows multiple extruders to be added easily.
void bresenham(int x0, int y0, int z0, int a0, int b0, int x1, int y1, int z1,
		int a1, int b1, long speed, FILE *fout) {
	int dx, dy, dz, da, db; // Signed distances
	int sx, sy, sz, sa, sb; // Signs of the distances
	int adx, ady, adz, ada, adb; // Absolute distances
	int adx2, ady2, adz2, ada2, adb2; // Absolute distances * 2
	int err1, err2, err3, err4; // Error/Deviation Ratio
	int xIter, yIter, zIter, aIter, bIter; // Iteration Variables for axes
	int x_step, y_step, z_step, a_step, b_step;

	dx = x1 - x0;
	dy = y1 - y0;
	dz = z1 - z0;
	da = a1 - a0;
	db = b1 - b0;

	if (dx < 0)
		sx = -1;
	else
		sx = 1;

	if (dy < 0)
		sy = -1;
	else
		sy = 1;

	if (dz < 0)
		sz = -1;
	else
		sz = 1;

	if (da < 0)
		sa = -1;
	else
		sa = 1;

	if (db < 0)
		sb = -1;
	else
		sb = 1;

	adx = abs(dx);
	ady = abs(dy);
	adz = abs(dz);
	ada = abs(da);
	adb = abs(db);

	adx2 = 2 * adx;
	ady2 = 2 * ady;
	adz2 = 2 * adz;
	ada2 = 2 * ada;
	adb2 = 2 * adb;

	// If x is the major axis
	if (adx >= ady && adx >= adz && adx >= ada && adx >= adb) {
		int x_step, y_step, z_step, a_step, b_step;
		err1 = ady2 - adx; // err1 = ad2
		err2 = adz2 - adx; // err2 = ad3
		err3 = ada2 - adx; // err3 = ad4
		err4 = adb2 - adx; // err4 = ad5

		int i;
		for (i = 0; i < adx; i++) {
			x_step = 0;
			y_step = 0;
			z_step = 0;
			a_step = 0;
			b_step = 0;

			if (err1 > 0) {
				yIter += sy;
				y_step = sy;
				err1 -= adx2;
			}

			if (err2 > 0) {
				zIter += sz;
				z_step = sz;
				err2 -= adx2;
			}

			if (err3 > 0) {
				aIter += sa;
				a_step = sa;
				err3 -= adx2;
			}

			if (err4 > 0) {
				bIter += sb;
				b_step = sb;
				err4 -= adx2;
			}

			err1 += ady2;
			err2 += adz2;
			err3 += ada2;
			err4 += adb2;

			xIter += sx;
			x_step = sx;

			write_to_fiq(x_step, y_step, z_step, a_step, b_step, speed, fout);

		}
	}

	// If y is the major axis
	if (ady > adx && ady >= adz && ady >= ada && ady >= adb) {
		x_step = 0;
		y_step = 0;
		z_step = 0;
		a_step = 0;
		b_step = 0;

		err1 = adx2 - ady;
		err2 = adz2 - ady;
		err3 = ada2 - ady;
		err4 = adb2 - ady;

		int i;
		for (i = 0; i < ady; i++) {
			x_step = 0;
			y_step = 0;
			z_step = 0;
			a_step = 0;
			b_step = 0;

			if (err1 > 0) {
				xIter += sx;
				x_step = sx;
				err1 -= ady2;
			}

			if (err2 > 0) {
				zIter += sz;
				z_step = sz;
				err2 -= ady2;
			}

			if (err3 > 0) {
				aIter += sa;
				a_step = sa;
				err3 -= ady2;
			}

			if (err4 > 0) {
				bIter += sb;
				b_step = sb;
				err4 -= ady2;
			}

			err1 += adx2;
			err2 += adz2;
			err3 += ada2;
			err4 += adb2;

			yIter += sy;
			y_step = sy;

			write_to_fiq(x_step, y_step, z_step, a_step, b_step, speed, fout);
		}
	}

	// If z is the major axis
	if (adz > adx && adz > ady && adz >= ada && adz >= adb) {
		x_step = 0;
		y_step = 0;
		z_step = 0;
		a_step = 0;
		b_step = 0;

		err1 = ady2 - adz;
		err2 = adx2 - adz;
		err3 = ada2 - adz;
		err4 = adb2 - adz;

		int i;
		for (i = 0; i < adz; i++) {
			x_step = 0;
			y_step = 0;
			z_step = 0;
			a_step = 0;
			b_step = 0;

			if (err1 > 0) {
				yIter += sy;
				y_step = sy;
				err1 -= adz2;
			}

			if (err2 > 0) {
				xIter += sx;
				x_step = sx;
				err2 -= adz2;
			}

			if (err3 > 0) {
				aIter += sa;
				a_step = sa;
				err3 -= adz2;
			}

			if (err4 > 0) {
				bIter += sb;
				b_step = sb;
				err4 -= adz2;
			}

			err1 += ady2;
			err2 += adx2;
			err3 += ada2;
			err4 += adb2;

			zIter += sz;
			z_step = sz;

			write_to_fiq(x_step, y_step, z_step, a_step, b_step, speed, fout);

		}
	}

	// If a is the major axis
	if (ada > adx && ada > ady && ada > adz && ada >= adb) {
		x_step = 0;
		y_step = 0;
		z_step = 0;
		a_step = 0;
		b_step = 0;

		err1 = adx2 - ada;
		err2 = ady2 - ada;
		err3 = adz2 - ada;
		err4 = adb2 - ada;

		int i;
		for (i = 0; i < ada; i++) {
			x_step = 0;
			y_step = 0;
			z_step = 0;
			a_step = 0;
			b_step = 0;

			if (err1 > 0) {
				xIter += sx;
				x_step = sx;
				err1 -= ada2;
			}

			if (err2 > 0) {
				yIter += sy;
				y_step = sy;
				err2 -= ada2;
			}

			if (err3 > 0) {
				zIter += sz;
				z_step = sz;
				err3 -= ada2;
			}

			if (err4 > 0) {
				bIter += sb;
				b_step = sb;
				err4 -= ada2;
			}

			err1 += adx2;
			err2 += ady2;
			err3 += adz2;
			err4 += adb2;

			aIter += sa;
			a_step = sa;
			write_to_fiq(x_step, y_step, z_step, a_step, b_step, speed, fout);

		}
	}

	if (adb > adx && adb > ady && adb > adz && adb > ada) {
		x_step = 0;
		y_step = 0;
		z_step = 0;
		a_step = 0;
		b_step = 0;

		err1 = adx2 - adb;
		err2 = ady2 - adb;
		err3 = adz2 - adb;
		err4 = ada2 - adb;

		int i;
		for (i = 0; i < adb; i++) {
			x_step = 0;
			y_step = 0;
			z_step = 0;
			a_step = 0;
			b_step = 0;

			if (err1 > 0) {
				xIter += sx;
				x_step = sx;
				err1 -= adb2;
			}

			if (err2 > 0) {
				yIter += sy;
				y_step = sy;
				err2 -= adb2;
			}

			if (err3 > 0) {
				zIter += sz;
				z_step = sz;
				err3 -= adb2;
			}

			if (err4 > 0) {
				bIter += sb;
				b_step = sb;
				err4 -= adb2;
			}

			err1 += adx2;
			err2 += ady2;
			err3 += adz2;
			err4 += ada2;

			bIter += sb;
			b_step = sb;

			write_to_fiq(x_step, y_step, z_step, a_step, b_step, speed, fout);
		}
	}
}

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

#if(QLZ_STREAMING_BUFFER == 0)
#error Define QLZ_STREAMING_BUFFER to a non-zero value for this demo
#endif

// Stolen from quicklz site. http://www.quicklz.com/stream_compress.c. It compresses the input, and returns the amount of bytes saved.
// Quicklz is awesome. It is not bloated, does the job quickly, compresses well, and is free.
unsigned long compressFile(FILE *ifile, FILE *ofile) {
	char *file_data, *compressed;
	size_t d, c;
	unsigned long bytes_saved = 0;
	qlz_state_compress *state_compress = (qlz_state_compress *) malloc(
			sizeof(qlz_state_compress));

	// allocate "uncompressed size" + 400 bytes for the destination buffer where
	// "uncompressed size" = 10000 in worst case in this sample demo
	file_data = (char *) malloc(10000);
	compressed = (char *) malloc(10000 + 400);

	// allocate and initially zero out the states. After this, make sure it is
	// preserved across calls and never modified manually
	memset(state_compress, 0, sizeof(qlz_state_compress));

	// compress the file in random sized packets
	while ((d = fread(file_data, 1, rand() % 10000 + 1, ifile)) != 0) {
		c = qlz_compress(file_data, compressed, d, state_compress);
		//printf("%u bytes compressed into %u\n", (unsigned int)d, (unsigned int)c);
		bytes_saved += ((unsigned int) d - (unsigned int) c);
		// the buffer "compressed" now contains c bytes which we could have sent directly to a
		// decompressing site for decompression
		fwrite(compressed, c, 1, ofile);
	}

	free(file_data);
	free(compressed);
	return bytes_saved;
}

double fiveDimensionalDistanceInt(int q[5],int p[5]){
	double distance = 0;
	for(int i=0;i<5;i++){
		distance += ((q[i] - p[i]) * (q[i] - p[i]));
	}
	return pow(distance,0.5);
}

double fiveDimensionalDistanceDouble(double q[5],double p[5]){
	double distance = 0;
	for(int i=0;i<5;i++){
		distance += ((q[i] - p[i]) * (q[i] - p[i]));
	}
	return pow(distance,0.5);
}