/*
 * Simple, but very fast converter of file to C++ array; written in old school C.
 * ehg.c (ehg.exe)
 * Based on https://github.com/TheLivingOne/bin2array/
 * by (C) Sergey A. Galin, 2019, sergey.galin@gmail.com, sergey.galin@yandex.ru
 * and compiled with TynyCC https://bellard.org/tcc/
 * This file is a Public Domain.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* replace_char(char* str, char find, char replace){
    char *current_pos = strchr(str,find);
    for (char* p = current_pos; (current_pos = strchr(str, find)) != NULL; *current_pos = replace);
    return str;
}

int main(int argc, char * argv[])
{
    if ((argc > 3)||(argc < 2)) {
        printf("USAGE: %s <input file> [PROGMEM]\n", argv[0]);
        return 1;
    }
	
    const char * in = argv[1];
	const char * pr = argv[2];
    char pr_o[8] = " ";
	
	if (argv[2]) sprintf(pr_o, "%s", pr);

    // Hello stack overflow :)
    char out_cpp[4096];
	char usname[4096];
	sprintf(usname, "%s", in);
    sprintf(out_cpp, "%s.h", in);
	
	replace_char(usname,'.', '_');
	
    printf("Input: %s, output: %s\n", in, out_cpp);
	
    //
    // Working with the input file
    //
    FILE * fin = fopen(in, "rb");
    if (!fin) {
        printf("Error opening input file!\n");
        return 2;
    }
    fseek(fin, 0, SEEK_END);
    size_t size = (size_t)ftell(fin);
    fseek(fin, 0, SEEK_SET);
    printf("Input data size: %ld\n", (long)size);

    unsigned char * data = malloc(size);
    if (fread(data, size, 1, fin) != 1) {
        printf("Failed to read input file!\n");
        free(data);
        fclose(fin);
        return 2;
    }
    fclose(fin);
	
	unsigned char arr[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    unsigned char * text = malloc(size * 6);
    unsigned char * out_ptr = text;
    unsigned char * in_ptr = data;
    for (size_t i = 1, col = 1; i <= size; i++, in_ptr++) {
        unsigned char x = *in_ptr;
		unsigned char y = (x & 0xF0) >> 4;
		unsigned char z = (x & 0x0F);
		
		*out_ptr++ = '0';
		*out_ptr++ = 'x';
	
		*out_ptr++ = arr[y];
		*out_ptr++ = arr[z];

        if (i != size) {
            *out_ptr++ = ',';
        }
        if (col == 20) {
            *out_ptr++ = '\n';
            col = 1;
        } else
            col++;
    }
    free(data);
    // *out_ptr = 0; not necessary as we're using fwrite()

    //
    // Writing output file
    //

    FILE * fout = fopen(out_cpp, "wb");
    if (!fout) {
        printf("Error opening output file!\n");
        free(text);
        return 2;
    }
    fprintf(
        fout,
		"\n//File: %s, Size: %ld\n"
		"#define %s_len %ld\n"
		"const uint8_t %s[] %s = {\n",
		in,
		(long)size,
		usname,
		(long)size,
		usname,
		pr_o);
    if (fwrite(text, out_ptr - text, 1, fout) != 1) {
        printf("Error writing output file!");
        free(text);
        fclose(fout);
        return 2;
    }
    fprintf(fout, "\n};\n");
    fclose(fout);
    free(text);
    return 0;
}

