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
#include "ArmStrongFiq.h"
#include "quicklz.h"

bool isVerbose = false;
bool isDecompressing = false;
char *inputFile;

int main(int argc, char *argv[])
{
    opterr = 0;
    int c;
    while ((c = getopt(argc, argv, "cvi:")) != -1)
    {
        switch (c)
        {
        case 'c':
            isDecompressing = true;
            break;
        case 'v':
            isVerbose = true;
            break;
        case 'i':
            if (access(optarg, F_OK) != -1)
            {
                inputFile = optarg;
                break;
            }
            else
            {
                printf("Source File Doesn't Exits: %s\nAborting\n", optarg);
                exit(-2);
            }
        }
    }

    if (optind < argc || argc == 1)
    {
        printf("\n%s: invalid usage\n", argv[0]);
        printf("\nusage: %s [-i INPUT -v -c]\n\nprints the fcode\n\n", argv[0]);
        printf("\t\t-i INPUT\t\tthe file to read from\n");
        printf("\t\t-v \t\t\tdisplay extra information (be verbose)\n");
        printf("\t\t-c \t\t\tthe input is compressed, and needs to be decompressed\n\n\n");
        exit(-4);
    }

    if (isDecompressing == true)
    {
        FILE *fsource = fopen(inputFile, "rb");
        FILE *fdest = fopen("temp.fcode", "wb");

        int expanded = decompressFile(fsource, fdest);
        fclose(fsource);
        fclose(fdest);
        if (isVerbose)
            printf("Expanded %i bytes\n", expanded);
    }

    if (isDecompressing == true)
    {
        //system("rm temp.fcode");
    }
}

int decompressFile(FILE *ifile, FILE *ofile)
{
    int byte_count = 0;
    char *file_data, *decompressed;
    size_t d, c;
    qlz_state_decompress *state_decompress = (qlz_state_decompress *)malloc(sizeof(qlz_state_decompress));

    // a compressed packet can be at most "uncompressed size" + 400 bytes large where
    // "uncompressed size" = 10000 in worst case in this sample demo
    file_data = (char *) malloc(10000 + 400);

    // allocate decompression buffer
    decompressed = (char *) malloc(10000);

    // allocate and initially zero out the scratch buffer. After this, make sure it is
    // preserved across calls and never modified manually
    memset(state_decompress, 0, sizeof(qlz_state_decompress));

    // read 9-byte header to find the size of the entire compressed packet, and
    // then read remaining packet
    while ((c = fread(file_data, 1, 9, ifile)) != 0)
    {
        c = qlz_size_compressed(file_data);
        fread(file_data + 9, 1, c - 9, ifile);
        d = qlz_decompress(file_data, decompressed, state_decompress);

        byte_count += (d - c);
        fwrite(decompressed, d, 1, ofile);
    }

    return byte_count;
}


// this section of code was written by Brian Lilly. it initializes the motors
void initialize_motors()
{
    system(
        // Turn on 12v
        "echo 243 > /sys/class/gpio/export;"
        "echo 1 > /sys/class/gpio/gpio243/value;"

        // turn on the motors
        // take the shift registers out of reset
        "echo 125 > /sys/class/gpio/export;"
        "echo out > /sys/class/gpio/gpio125/direction;"
        "echo 1 > /sys/class/gpio/gpio125/value;"
        "sleep 1;"
        "echo 208 > /sys/class/gpio/export;"
        "echo 209 > /sys/class/gpio/export;"
        "echo 210 > /sys/class/gpio/export;"
        "echo 211 > /sys/class/gpio/export;"
        "echo 212 > /sys/class/gpio/export;"
        "echo 213 > /sys/class/gpio/export;"
        "echo 214 > /sys/class/gpio/export;"
        "echo 215 > /sys/class/gpio/export;"
        "echo 216 > /sys/class/gpio/export;"
        "echo 217 > /sys/class/gpio/export;"
        "echo 218 > /sys/class/gpio/export;"
        "echo 219 > /sys/class/gpio/export;"
        "echo 220 > /sys/class/gpio/export;"
        "echo 221 > /sys/class/gpio/export;"
        "echo 222 > /sys/class/gpio/export;"
        "echo 223 > /sys/class/gpio/export;"
        "echo 224 > /sys/class/gpio/export;"
        "echo 225 > /sys/class/gpio/export;"
        "echo 226 > /sys/class/gpio/export;"
        "echo 227 > /sys/class/gpio/export;"
        "echo 228 > /sys/class/gpio/export;"
        "echo 229 > /sys/class/gpio/export;"
        "echo 230 > /sys/class/gpio/export;"
        "echo 231 > /sys/class/gpio/export;"
        "echo 232 > /sys/class/gpio/export;"
        "echo 233 > /sys/class/gpio/export;"
        "echo 234 > /sys/class/gpio/export;"
        "echo 235 > /sys/class/gpio/export;"
        "echo 236 > /sys/class/gpio/export;"
        "echo 237 > /sys/class/gpio/export;"
        "echo 238 > /sys/class/gpio/export;"
        "echo 239 > /sys/class/gpio/export;"

        // turn on the light switch
        "echo 242 > /sys/class/gpio/export;"

        // flick the switch
        "echo 1 > /sys/class/gpio/gpio242/value;"

        // set the reset bits
        "echo 1 > /sys/class/gpio/gpio208/value;"
        "echo 1 > /sys/class/gpio/gpio214/value;"
        "echo 1 > /sys/class/gpio/gpio220/value;"
        "echo 1 > /sys/class/gpio/gpio226/value;"
        "echo 1 > /sys/class/gpio/gpio232/value;"

        // set the enable bits
        "echo 0 > /sys/class/gpio/gpio209/value;"
        "echo 0 > /sys/class/gpio/gpio215/value;"
        "echo 0 > /sys/class/gpio/gpio221/value;"
        "echo 0 > /sys/class/gpio/gpio227/value;"
        "echo 0 > /sys/class/gpio/gpio233/value;"

        // set the sleep bits
        "echo 1 > /sys/class/gpio/gpio210/value;"
        "echo 1 > /sys/class/gpio/gpio216/value;"
        "echo 1 > /sys/class/gpio/gpio222/value;"
        "echo 1 > /sys/class/gpio/gpio226/value;"
        "echo 1 > /sys/class/gpio/gpio234/value;"

        // set the stepper rate
        "echo 1 > /sys/class/gpio/gpio211/value;"
        "echo 1 > /sys/class/gpio/gpio212/value;"
        "echo 1 > /sys/class/gpio/gpio213/value;"
        "echo 1 > /sys/class/gpio/gpio217/value;"
        "echo 1 > /sys/class/gpio/gpio218/value;"
        "echo 1 > /sys/class/gpio/gpio219/value;"
        "echo 1 > /sys/class/gpio/gpio223/value;"
        "echo 1 > /sys/class/gpio/gpio224/value;"
        "echo 1 > /sys/class/gpio/gpio225/value;"
        "echo 1 > /sys/class/gpio/gpio229/value;"
        "echo 1 > /sys/class/gpio/gpio230/value;"
        "echo 1 > /sys/class/gpio/gpio231/value;"
        "echo 1 > /sys/class/gpio/gpio235/value;"
        "echo 1 > /sys/class/gpio/gpio236/value;"
        "echo 1 > /sys/class/gpio/gpio237/value;"

    );
    // set the current of each stepper

    //"./test_spidev2 180 8;"
    //"./test_spidev2 180 4;"
    //"./test_spidev2 180 12 ;"
    //"./test_spidev2 180 2;"


}