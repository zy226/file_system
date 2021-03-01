#include <stdint.h>

#define INODE_MAP_LEN 32
#define BLOCK_MAP_LEN 128
#define INODE_MAX 1024
#define BLOCK_MAX 4096

typedef struct super_block {
    int32_t magic_num;                  // 幻数
    int32_t free_block_count;           // 空闲数据块数
    int32_t free_inode_count;           // 空闲inode数
    int32_t dir_inode_count;            // 目录inode数
    uint32_t block_map[128];            // 数据块占用位图
    uint32_t inode_map[32];             // inode占用位图
} sp_block;

typedef struct inode {
    uint32_t size;              // 文件大小
    uint16_t file_type;         // 文件类型（文件0/文件夹1）
    uint16_t link;              // 连接数
    uint32_t block_point[6];    // 数据块指针
}inode_t;

typedef struct dir_item {               // 目录项一个更常见的叫法是 dirent(directory entry)
    uint32_t inode_id;          // 当前目录项表示的文件/目录的对应inode
    uint16_t valid;             // 当前目录项是否有效 
    uint8_t type;               // 当前目录项类型（文件/目录）
    char name[121];             // 目录项表示的文件/目录的文件名/目录名
}dir_item_t;

void boot_load();
uint32_t inode_find_free();
uint32_t block_find_free();
uint32_t fd_create(uint32_t parent_inode_num,char *file_name,uint16_t file_type);
uint32_t dir_list(uint32_t inode_num);
uint32_t file_copy(uint32_t src_num,uint32_t dst_num,char *dst_name);

uint32_t open_by_path(char *path);
uint32_t open_by_inode(char *path,uint32_t inode_num);
