#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <getopt.h>

bool is_verbose = false;
char *source_file;
char *dest_file_folder;
int max_chunk_size;

int main(int argc, char *argv[])
{

    opterr = 0;
    int c;
    while ((c = getopt(argc, argv, "vi:o:s:")) != -1)
    {
        switch (c)
        {
        case 'v':
        {
            is_verbose = true;
            break;
        }
        case 'i':
        {
            if (access(optarg, F_OK) != -1)
            {
                source_file = optarg;
                break;
            }
            else
            {
                printf("source file doesn't exits: %s\naborting\n", optarg);
                exit(-2);
            }
        }
        case 'o':
        {
            if (access(optarg, F_OK) != -1)
            {
                printf("file already exists\n");
                exit(-1);
            }
            else
            {
                dest_file_folder = optarg;
                break;
            }
        }
        case 's':
        {
            int i;
            int result = sscanf(optarg, "%i", &i);

            if (result != 1)
            {
                printf("invalid file size\n");
                exit(-1);
            }
            else
            {
                max_chunk_size = i * 1024 * 1024;
                break;
            }
        }

        }
    }

    if (optind < argc || argc == 1)
    {
        printf("\n%s: invalid usage\n", argv[0]);
        printf("\nusage: %s [-i INPUT -o OUTPUT -v -c]\n\nsplits a file into smaller files\n\n", argv[0]);
        printf("\t\t-i INPUT\t\tthe file to read from\n");
        printf("\t\t-o OUTPUT\t\tthe folder to create and write to\n");
        printf("\t\t-s SIZE\t\tthe size to split the chunks into in MB\n");
        printf("\t\t-v \t\t\tdisplay extra information (be verbose)\n");

        exit(-4);
    }

    if(max_chunk_size == 0 ){
    	printf("invalid chunk size:\t%i\n",max_chunk_size);
    	exit(-3);
    }


    char cmd[128];
    sprintf(cmd, "mkdir %s", dest_file_folder);
    system(cmd);


    int file_number = 0;
    int file_size = -1;
    int bytes_writen_count = 0;
    int file_actual_size = 0;

    FILE *fin = fopen(source_file, "rb");

    fseek (fin , 0 , SEEK_END);
    file_actual_size = ftell (fin);
    rewind (fin);

    FILE *fout = NULL;
    while (bytes_writen_count < file_actual_size)
    {
        if (file_size == -1 || file_size >= max_chunk_size)
        {
            if (fout != NULL)
                fclose(fout);
            printf("FSize %i,MaxChunkSize %i\n",file_size,max_chunk_size);
            char new_file_name[128];
            printf("%s/source_%i.fcode\n", dest_file_folder, file_number);
            sprintf(new_file_name, "./%s/source_%i.fcode", dest_file_folder, file_number++);
            
            fout = fopen(new_file_name, "wb");
            file_size = 0;
        }
        unsigned long bit32in;
        fread(&bit32in, 12, 1, fin);
        fwrite(&bit32in, 12, 1, fout);
        file_size += 12;
        bytes_writen_count += 12;
    }

    fclose(fin);
    fclose(fout);
}