/**
 * @file gen_patch.c
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

#include "bsdiff.h"

#include "bitio.h"
#include "lz77.h"

#define GENERATE_PATCH


typedef struct bsdiff_stream bstream_t;

/**
 * @brief 
 * 
 * @param stream 
 * @param buffer 
 * @param size 
 * @return int 
 * -1 err
 * 0 ok
 */
static int __bsdiff_write(bstream_t* stream, const void* buffer, int size)
{
    int ret = 0;
    FILE *tmpf = NULL;
    tmpf = (FILE *)stream->opaque;
    ret = fwrite(buffer, sizeof(char), size, tmpf);
    if(ret != size){
        perror("write to diff_data to tmp file.");
        return -1;
    }
    return 0;
}
/**
 * @brief trans ing64 to u8 array
 * 
 * @param x 
 * @param buf 
 */
static void offout(int64_t x,uint8_t *buf)
{
	int64_t y;

	if(x<0) y=-x; else y=x;

	buf[0]=y%256;y-=buf[0];
	y=y/256;buf[1]=y%256;y-=buf[1];
	y=y/256;buf[2]=y%256;y-=buf[2];
	y=y/256;buf[3]=y%256;y-=buf[3];
	y=y/256;buf[4]=y%256;y-=buf[4];
	y=y/256;buf[5]=y%256;y-=buf[5];
	y=y/256;buf[6]=y%256;y-=buf[6];
	y=y/256;buf[7]=y%256;

	if(x<0) buf[7]|=0x80;
}

#ifdef GENERATE_PATCH
/**
 * @brief generate patch file from old file and new file
 * 
 * @param argc 
 * @param argv 
 * options:-n: input new file path
 * options:-o: input old file path
 * options:-p: get patch file path after compression with lz77
 * options:-h: help
 * @return int 
 */
int main(int argc, char *argv[])
{
    int ret;
    int opt;
    FILE *nf = NULL, *of = NULL, *patchf = NULL, *tmpf = NULL;
    const char* tmpfile_path = "./diff_file";
    char *newfile_path = NULL, *oldfile_path = NULL, *patchfile_path = NULL;
    
    //define bsdiff relevant data
    int oldsize;
    int newsize;
    int diffsize;
    bstream_t bs_s;
    uint8_t *old = NULL, *new = NULL, buf[8], buf_o[8];
    

    //define lz77 relevant data
    struct bitFILE *bitF = NULL;
    int patchsize;

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
                printf(" -n  :  new input file to diff.\n");
                printf(" -o  :  old input file to compare diff with new file.\n");
                printf(" -p  :  get patch file after compare and compression with lz77.\n");
                printf(" -h  :  Command line options.\n\n");
                break;
        }
    }
    if(newfile_path == NULL || oldfile_path == NULL || patchfile_path == NULL){
        perror("None file");
        goto err;
    }
    nf = fopen(newfile_path, "rb");
    if(nf == NULL){
        perror("Opening new file");
        goto err;
    }
    of = fopen(oldfile_path, "rb");
    if(of == NULL){
        perror("Opening old file");
        goto err;
    }



    tmpf = fopen(tmpfile_path, "wb+");
    if(tmpf == NULL){
        perror("Opening tmp file");
        goto err;
    }
    
    //bsdiff
    bs_s.malloc = malloc;
    bs_s.free = free;
    bs_s.write = __bsdiff_write;
    
    //read old file
    ret = fseek(of, 0, SEEK_END);
    if( ret ){
        err(1,"read %s",oldfile_path);
        goto err;
    }
    oldsize = ftell(of);
    old = malloc(oldsize + 1);
    if(oldsize == 0 || old == 0){
        err(1,"read %s",oldfile_path);
        goto err;
    }
    fseek(of, 0, SEEK_SET);
    fread(old, 1, oldsize, of);
    fclose(of);
    of = NULL;


    //read new file
    ret = fseek(nf, 0, SEEK_END);
    if(ret != 0){
        err(1,"read %s",newfile_path);
        goto err;
    }
    newsize = ftell(nf);
    new = malloc(newsize + 1);
    if(newsize == 0 || new == 0){
        err(1,"read %s",newfile_path);
        goto err;
    }
    fseek(nf, 0, SEEK_SET);
    fread(new, 1, newsize, nf);
    fclose(nf);
    nf = NULL;

    //write head(signature and newfile size)
    offout(newsize, buf);
    offout(oldsize, buf_o);
    if(fwrite("ENDSLEY/BSDIFF43", 16, sizeof(char), tmpf) != 1 ||
       fwrite(buf, sizeof(buf), sizeof(char), tmpf) != 1  ||
       fwrite(buf_o, sizeof(buf_o), sizeof(char), tmpf) != 1  ||){

        perror("Failed to write tmp diff file header.");
        goto err;
        
    }
    
    bs_s.opaque = tmpf;//tempe diff file before compression

    if(bsdiff(old, oldsize, new, newsize, &bs_s) < 0){
        perror("bsdiff");
        goto err;
    }else{
        ret = fseek(tmpf, 0, SEEK_END);
        diffsize = ftell(tmpf);
        if(tmpf){
            fclose(tmpf);
            tmpf = NULL;
        }
    }
    //only read tmp diff file
    tmpf = fopen(tmpfile_path, "rb");

    //compress to patchfile with LZ77
    if ((bitF = bitIO_open(patchfile_path, BIT_IO_W)) == NULL) {
        perror("Opening output patchfile file");
        goto err;
    }
    //compress tmp diff file to patch file
    encode(tmpf, bitF, -1, -1);
    bitIO_close(bitF);

    patchf = fopen(patchfile_path, "rb");
    ret = fseek(patchf, 0, SEEK_END);
    patchsize = ftell(patchf);
    printf("bsdiff ok, compress ok\nRoot_path:%s\n",__FILE__);
    printf("new file %s size:%u Bytes\n", newfile_path, newsize);
    printf("old file %s size:%u Bytes\n", oldfile_path, oldsize);
    printf("diff file %s size:%u Bytes\n",tmpfile_path, diffsize);
    printf("patch file %s size:%u Bytes\n", patchfile_path, patchsize);
    
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
    if(tmpf){
        fclose(tmpf);
    }
    return 0;
}
#endif


