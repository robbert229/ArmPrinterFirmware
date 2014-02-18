void parseG0(char *input, int lineNumber);
void parseG1(char *input, int lineNumber);
void parseG92(char *input, int lineNumber);
void parseG28(char *input, int lineNumber);
void breshenham(int x0, int y0, int z0, int a0, int b0, int x1, int y1, int z1,
		int a1, int b1, long speed, FILE *fout);
void fwrite32(uint32_t val, FILE *f);
void write_to_fiq(int x, int y, int z, int a, int b, long speed, FILE *fout);
int fLineCount(FILE *file);
double fiveDimensionalDistanceInt(int[5] ,int[5]);
double fiveDimensionalDistanceDouble(double[5],double[5]);
unsigned long compressFile(FILE *ifile, FILE *ofile);
