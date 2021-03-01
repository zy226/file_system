#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "file.h"
#include "disk.h"

void disk_read(uint32_t num,char *tempbuf){
    disk_read_block(num*2,tempbuf);
    disk_read_block(num*2+1,tempbuf+DEVICE_BLOCK_SIZE);
}

void disk_write(uint32_t num,char *tempbuf){
    disk_write_block(num*2,tempbuf);
    disk_write_block(num*2+1,tempbuf+DEVICE_BLOCK_SIZE);
}
// Return 1 if bit at position index in array is set to 1

int bit_isset(uint32_t *array, int index)
{
    uint32_t b = array[index/32];
    uint32_t m = (1 << (index % 32));
    return (b & m) == m;
}
// Set bit at position index in array to 1

void bit_set(uint32_t *array, int index) {
  char b = array[index/32];
  char m = (1 << (index % 32));
  array[index/32] = (b | m);
}

uint32_t inode_find_free(){
    char *buf;
    sp_block *super_block;
    uint32_t i=0;
    uint32_t j=0;
    buf = (char*)malloc(DEVICE_BLOCK_SIZE * 2 + 1);
    disk_read(0,buf);
    super_block=(sp_block*)buf;
    for(i=0;i<INODE_MAP_LEN;i++){
        if(super_block->inode_map[i]<0xffffffff){
            break;
        }
    }
    if(i==INODE_MAP_LEN){
        j=0;
    }else{
        for (j = i*32; j < (i+1)*32; j++){
            if(!bit_isset(super_block->inode_map, j))
                break;
        }
    }
    free(buf);
    return j;
}

uint32_t block_find_free(){
    char *buf;
    sp_block *super_block;
    buf = (char*)malloc(DEVICE_BLOCK_SIZE * 2 + 1);
    disk_read(0,buf);
    super_block=(sp_block*)buf;
    uint32_t i=0;
    uint32_t j=0;
    for (i = 0; i < BLOCK_MAP_LEN; i++){
        if(super_block->block_map[i] < 0xffffffff)
            break;
    }
    if(i==BLOCK_MAP_LEN){
        j=0;
    }else{
        for (j = i*32; j < (i+1)*32; j++){
            if(!bit_isset(super_block->block_map, j))
                break;
        }
    }
    free(buf);
    return j;
}

uint32_t fd_create(uint32_t parent_inode_num,char *file_name,uint16_t file_type){
    char *block_buf;
    char *buf;
    sp_block *super_block;
    inode_t *inode;
    dir_item_t *dir_item;
    uint32_t inode_num=inode_find_free();
    uint32_t block_num=0;
    uint32_t i,j;
    if(!inode_num){
        printf("none free inode");
        return 0; 
    }
    buf = (char*)malloc(DEVICE_BLOCK_SIZE * 2 + 1);
    disk_read(1 + parent_inode_num/32,buf);
    inode = (inode_t*)(buf + (parent_inode_num%32) * sizeof(inode_t));
    if(inode->file_type == 0){
        free(buf);
        return 0;
    }
    inode->size+=sizeof(dir_item_t);
    block_buf = (char*)malloc(DEVICE_BLOCK_SIZE * 2 + 1);
    //find same file name 
    for(i=0;i<6;i++){
        if(inode->block_point[i]>=33 && inode->block_point[i]<BLOCK_MAX){
            disk_read(inode->block_point[i],block_buf);
            for(j=0;j<DEVICE_BLOCK_SIZE/sizeof(dir_item_t);j++){
                dir_item = (dir_item_t*)(block_buf + j * sizeof(dir_item_t));
                if(dir_item->valid && !strcmp(dir_item->name,file_name)){
                    printf("same file name exits\n");
                    free(block_buf);
                    free(buf);
                    return 0;
                }
            }
        }
    }
    int flag =0;
    for(i=0;i<6;i++){
        if(inode->block_point[i] >= 33 && inode->block_point[i] < BLOCK_MAX){
            disk_read( inode->block_point[i],block_buf);
            for(j = 0; j < 2*DEVICE_BLOCK_SIZE / sizeof(dir_item_t); j++){
                dir_item = (dir_item_t*)(block_buf + j * sizeof(dir_item_t));
                if(dir_item->valid == 0){
                    dir_item->inode_id = inode_num;
                    dir_item->valid = 1;
                    dir_item->type = file_type;
                    strcpy(dir_item->name, file_name);
                    disk_write(inode->block_point[i],block_buf);
                    flag = 1;
                    break;
                }
            }
        }
        else   
        {  
            block_num = block_find_free();
            if(!block_num){
                free(buf);
                printf("none free block\n");
                return 0;
            }
            inode->block_point[i] = block_num;
            disk_read(inode->block_point[i],block_buf);
            dir_item = (dir_item_t*)(block_buf);
            dir_item->inode_id = inode_num;
            dir_item->valid = 1;
            dir_item->type = file_type;
            strcpy(dir_item->name, file_name);
            disk_write(inode->block_point[i],block_buf);
            flag = 1;
        }
        if(flag)
            break;
    }
    free(block_buf);
    disk_write( 1 + parent_inode_num/32,buf);
    //update super block
    disk_read(0,buf);
    super_block = (sp_block*) buf;
    if(block_num!=-1){
        bit_set(super_block->block_map, block_num);
        super_block->free_block_count--;
    }
    super_block->free_inode_count--;
    bit_set(super_block->inode_map, inode_num);
    if(file_type==1)
        super_block->dir_inode_count++;
    disk_write(0,buf);
    
    //update file inode
    disk_read( 1 + inode_num/32,buf);
    inode = (inode_t*)(buf + (inode_num%32) * sizeof(inode_t));
    inode->size = 0;
    inode->file_type = file_type;
    inode->link = 1;
    for(i = 0; i < 6; i++)
        inode->block_point[i] = 0;
    disk_write( 1 + inode_num/32,buf);
    free(buf);
    return inode_num;
}

uint32_t dir_list(uint32_t inode_num){
    inode_t *inode;
    char *buf;
    dir_item_t *dir_item;
    uint32_t i,j;
    buf = (char*)malloc(DEVICE_BLOCK_SIZE * 2 + 1);
    inode = (inode_t*)(buf + (inode_num%32) * sizeof(inode_t));
    disk_read( 1 + inode_num/32,buf);
    if(inode->file_type==0){
        printf("can't open a file\n");
        return 0;
    }
    for(i=0;i<6;i++){
         if(inode->block_point[i]>=33&&inode->block_point[i]<BLOCK_MAX){
            disk_read( inode->block_point[i],buf);
            for(j = 0; j < 2*DEVICE_BLOCK_SIZE / sizeof(dir_item_t); j++){
                dir_item = (dir_item_t*)(buf + j * sizeof(dir_item_t));
                if(dir_item->valid)
                    printf("%s\n", dir_item->name);
            }
         }
    }
    free(buf);
    return 0;
}

uint32_t file_copy(uint32_t src_num,uint32_t dst_num,char *dst_name){
    inode_t *src_inode;
    inode_t *dst_inode;
    char *block_buf;
    uint32_t i;
    uint32_t inode_num;
    uint32_t block_num=-1;
    char *buf;

    buf = (char*)malloc(DEVICE_BLOCK_SIZE * 2 + 1);
    disk_read(1 + src_num/32,buf);
    src_inode = (inode_t*)(buf + (src_num%32) * sizeof(inode_t));
    if(src_inode->file_type==1){
        printf("can't copy a directory\n");
        free(buf);
        return 0;
    }
    inode_num=fd_create(dst_num,dst_name,0);
    if(!inode_num){
        printf("Create file failed\n");
        free(buf);
    }

    disk_read( 1 + inode_num/32,buf);
    dst_inode = (inode_t*)(buf + (inode_num%32) * sizeof(inode_t));
    dst_inode->size = src_inode->size;
    dst_inode->file_type = src_inode->file_type;
    dst_inode->link = src_inode->file_type;
    block_buf = (char*)malloc(DEVICE_BLOCK_SIZE * 2 + 1);
    for(i=0;i<6;i++){
        if(src_inode->block_point[i]>=33 && src_inode->block_point[i]<BLOCK_MAX){
            block_num = block_find_free();
            if(!block_num){
                return 0;
            }
            dst_inode->block_point[i] = block_num;
            disk_read(src_inode->block_point[i],block_buf);
            disk_write(dst_inode->block_point[i],block_buf);
        }
    }
    disk_write(1+inode_num/32,buf);
    free(buf);
    free(block_buf);
    return inode_num;
}

uint32_t open_by_path(char *path){
    if(path[0] == '/')
        return open_by_inode(path+1,2);
    else
        return open_by_inode(path,2);
}

uint32_t open_by_inode(char *path,uint32_t inode_num){
    inode_t *inode;
    dir_item_t *dir_item;
    uint32_t i,j;
    char *buf;
    char *p;
    p = path;
    if(path[0] == 0)
        return inode_num;
    while(p[0] != '/' && p[0] != 0)
        p++;
    buf = (char*)malloc(DEVICE_BLOCK_SIZE * 2 + 1);
    disk_read( 1 + inode_num/32,buf);
    inode = (inode_t*)(buf + (inode_num%32) * sizeof(inode_t));
    for(i=0;i<6;i++){
        if(inode->block_point[i]>=33&&inode->block_point[i]<BLOCK_MAX){
            disk_read(inode->block_point[i],buf);
            for(j = 0; j < 2*DEVICE_BLOCK_SIZE / sizeof(dir_item_t); j++){
                dir_item = (dir_item_t*)(buf + j * sizeof(dir_item_t));
                if(dir_item->valid){
                    if(*p == '/'){
                        *p =0;
                        p++;
                    }
                if(!strcmp(dir_item->name, path)){
                    inode_num =dir_item->inode_id;
                    free(buf);
                    return open_by_inode(p,inode_num);
                    }
                }
            }
        }
    }
    free(buf);
    return 0;
}

void fs_init(){
    char *buf;
    sp_block *super_block;
    inode_t *inode;
    int i;
    buf=(char*)malloc(DEVICE_BLOCK_SIZE*2+1);
    disk_read(0,buf);
    super_block = (sp_block*)buf;
    super_block->magic_num=0xdec0de;
    super_block->free_block_count=BLOCK_MAX-33;
    super_block->free_inode_count=INODE_MAX-1;
    super_block->dir_inode_count=1;

    memset(super_block->block_map,0,BLOCK_MAP_LEN);
    memset(super_block->inode_map,0,INODE_MAP_LEN);

    for(i=0;i<=INODE_MAP_LEN;i++){
        bit_set(super_block->block_map,i);
    }
    for(i=0;i<=2;i++){
        bit_set(super_block->inode_map,i);
    }
    disk_write(0,buf);
    disk_read(1,buf);
    inode = (inode_t*)(buf + 2*sizeof(inode_t));
    inode->size=0;
    inode->link=1;
    inode->file_type=1;
    disk_write(1,buf);
    free(buf);
}

void boot_load(){
    sp_block *super_block;
    char *buf;
    char format ='a';
    if(open_disk() < 0){
        printf("Open disk failed!\n");
        exit(-1);
    }

    buf=(char*)malloc(DEVICE_BLOCK_SIZE*2+1);
    disk_read(0,buf);
    super_block=(sp_block*)buf;
    if(super_block->magic_num == 0xdec0de){
        printf("system found\n");
        printf("Rebuild file system?(y/n) ");
        format=getchar();
        while (format != 'y' && format != 'n')
            format=getchar();
        if(format == 'y'){
            FILE* tmp = fopen("disk","w");
            for(int32_t i = 0; i < get_disk_size(); i++){
                fputc(0,tmp);
            }
            fclose(tmp);
            fs_init();
        }
        
    }else{
        fs_init();
    }
    free(buf);
}