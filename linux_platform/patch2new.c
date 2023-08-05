/**
 * @file patch2new.c
 * @author Leo-jiahao (leo884823525@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-04
 * 
 * @copyright 
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) <2023>  <Leo-jiahao> 
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 */
/*Include --------------------------------------------------*/
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>

#include "bspatch.h"

#include "bitio.h"
#include "lz77.h"


#define PATCH2NEW


/**
 * @brief trans u8 array to int64
 * 
 * @param buf 
 * @return int64_t 
 */
static int64_t offtin(uint8_t *buf)
{
	int64_t y;

	y=buf[7]&0x7F;
	y=y*256;y+=buf[6];
	y=y*256;y+=buf[5];
	y=y*256;y+=buf[4];
	y=y*256;y+=buf[3];
	y=y*256;y+=buf[2];
	y=y*256;y+=buf[1];
	y=y*256;y+=buf[0];

	if(buf[7]&0x80) y=-y;

	return y;
}
/**
 * @brief read data outt off decompression patch file of stream
 * 
 * @param stream 
 * @param buffer 
 * @param length 
 * @return int 
 */
static int _bspatch_read(const struct bspatch_stream* stream, void* buffer, int length)
{
    int ret = 0;
    FILE *decomf = NULL;
    decomf = (FILE *)stream->opaque;
    ret = fread(buffer, sizeof(char), length, decomf);
    if(ret != length){
        perror("read decompress patch file to buffer.");
        return -1;
    }
    return 0;
}


#ifdef PATCH2NEW
/**
 * @brief generate new file from patch and old file
 * 
 * @param argc 
 * @param argv 
 * options:-o :input old file path
 * options:-p :input patch file path
 * options:-n :new file to create, generate from old file and patch file
 * options:-h :help
 * @return int 
 */
int main(int argc, char *argv[])
{
    int64_t head_old_size = 0;
    int ret = 0, res = 0;
    int opt = 0;
    FILE *nf = NULL, *of = NULL, *patchf = NULL, *decomf = NULL;
    const char* decomf_path = "./decom_patch_file";
    char *newfile_path = NULL, *oldfile_path = NULL, *patchfile_path = NULL;
    int oldsize = 0, decomsize = 0, patchsize = 0;
    int64_t newsize = 0;
    uint8_t *old, *new;
    //decompress lz77
    struct bitFILE *bitF = NULL;

    //bs patch
    uint8_t header[24];
    struct bspatch_stream stream;

    while((opt = getopt(argc, argv, "n:o:p:h")) != -1){
        switch(opt){
            case 'n':
                if(newfile_path != NULL){
                    fprintf(stderr, "Multiple input files not allowed.\n");
                }
                newfile_path = malloc(strlen(optarg) + 1);
                strcpy(newfile_path, optarg);
                break;
            case 'o':
                if(oldfile_path != NULL){
                    fprintf(stderr, "Multiple input files not allowed.\n");
                }
                oldfile_path = malloc(strlen(optarg) + 1);
                strcpy(oldfile_path, optarg);
                break;
            case 'p':
                if(patchfile_path != NULL){
                    fprintf(stderr, "Multiple input files not allowed.\n");
                }
                patchfile_path = malloc(strlen(optarg) + 1);
                strcpy(patchfile_path, optarg);
                break;
            case 'h':
                printf("Usage:  lz77 <option>\n");
                printf(" -n  :  new file created to out put old file updata by patchfile.\n");
                printf(" -o  :  old input file.\n");
                printf(" -p  :  patch input file.\n");
                printf(" -h  :  Command line options.\n\n");
                break;
        }
    }
    if(newfile_path == NULL || oldfile_path == NULL || patchfile_path == NULL){
        perror("None file");
        ret = -1;
        goto err;
    }
    
    

    //decompress
    decomf = fopen(decomf_path, "wb+");
    if(decomf == NULL){
        perror("Opening or creating decompress file");
        ret = -1;
        goto err;
    }
    bitF = bitIO_open(patchfile_path, BIT_IO_R);
    if(bitF == NULL){
        perror("Opening patch file");
        ret = -1;
        goto err;
    }
    decode(bitF, decomf);
    bitIO_close(bitF);
    res = fseek(decomf,0 , SEEK_END);
    decomsize = ftell(decomf);
    res = fseek(decomf, 0, SEEK_SET);

    //patch

    //1: open old file, read old file
    of = fopen(oldfile_path, "rb");
    if(of == NULL){
        perror("Opening old file.");
        ret = -1;
        goto err;
    }
    res = fseek(of,0 , SEEK_END);
    oldsize = ftell(of);
    res = fseek(of,0 , SEEK_SET);
    old = malloc(oldsize + 1);
    if(old == NULL){
        perror("malloc old file buff.");
        ret = -1;
        goto err;
    }
    res = fread(old, sizeof(char), oldsize, of);
    if(res != oldsize){
        perror("Reading old file.");
        ret = -1;
        goto err;
    }

    //2: open new file
    nf = fopen(newfile_path, "wb+");
    if(nf == NULL){
        perror("Opening or creating new file");
        ret = -1;
        goto err;
    }
    res = fseek(nf,0 , SEEK_SET);

    //get new file size from decompresion patch file
    //(24 -> 32) add old file size information to head,
    res = fread(header, sizeof(char), 32, decomf);
    if(res != 32){
        perror("Corrupt patch file");
        ret = -1;
        goto err;
    }
    if(memcmp(header, "ENDSLEY/BSDIFF43", 16) != 0){
        perror("Corrupt patch file");
        ret = -1;
        goto err;
    }
    newsize = offtin(header+16);
    if(newsize < 0){
        perror("Corrupt patch file");
        ret = -1;
        goto err;
    }
    head_old_size = offtin(header+24);
    if(head_old_size < 0){
        perror("Corrupt patch file");
        ret = -1;
        goto err;
    }
    new = malloc(newsize + 1);
    if(new == NULL){
        perror("malloc new file buff.");\
        ret = -1;
        goto err;
    }

    //patch to new file
    stream.read = _bspatch_read;
	stream.opaque = decomf;
    
	if(bspatch(old, oldsize, new, newsize, &stream)){
        perror("bs patch.");
        ret = -1;
        goto err;
    }
    res = fwrite(new, sizeof(char), newsize,nf);
    if(res != newsize){
        perror("write to new file.");
        ret = -1;
        goto err;
    }

err:
    if(old){
        free(old);
    }
    if(new){
        free(new);
    }


    if(newfile_path){
        free(newfile_path);
    }
    if(oldfile_path){
        free(oldfile_path);
    }
    if(patchfile_path){
        free(patchfile_path);
    }

    if(nf){
        fclose(nf);
    }
    if(of){
        fclose(of);
    }
    if(patchf){
        fclose(patchf);
    }
    if(decomf){
        fclose(decomf);
    }
    return ret;
}
#endif


