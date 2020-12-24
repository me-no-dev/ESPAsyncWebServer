/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2015 Anthony Birkett
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * rehg.c (rehg.exe): Tool to reverse C-code array to file converted by ehg.exe
 * Based on https://github.com/birkett/cbintools/tree/master/c2bin
 * and compiled with TynyCC https://bellard.org/tcc/
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 500

int main(int argc, char *argv[])
{
    FILE *inputFile = NULL;
    FILE *outputFile = NULL;
    char sBuffer[BUFFER_SIZE];
    char *pch;
    char *newline, *endBracket;
    int  i;

    if (argc != 3)
    {
        printf("%s %s %s\n", "Usage:", argv[0], "<header> <output>");
        return 1;
    }

    inputFile = fopen(argv[1], "r");

    if (inputFile == NULL)
    {
        printf("%s %s\n", "Unable to open input header:", argv[1]);
        return 1;
    }

    outputFile = fopen(argv[2], "wb");

    if (outputFile == NULL)
    {
        printf("%s %s\n", "Unable to open output file:", argv[2]);
        return 1;
    }

    // Skip the first 4 lines.
    for (i = 0; i < 4; i++)
    {
        fgets(sBuffer, BUFFER_SIZE, inputFile);
    }

    // Get the contents of each line of the array.
    while (fgets(sBuffer, BUFFER_SIZE, inputFile))
    {
        // Get rid of the new line character.
        newline = strchr(sBuffer, '\n');
        if (newline)
        {
            *newline = 0;
        }

        // Skip this line if its the closing "};".
        endBracket = strchr(sBuffer, '}');
        if (endBracket)
        {
            continue;
        }

        // Write out each character.
        pch = strtok(sBuffer, ",");
        while (pch != NULL)
        {
			fprintf(outputFile, "%c", strtol(pch, NULL, 0)); // autodetect
            pch = strtok(NULL, ",");
        }
    }

    return 0;
}
