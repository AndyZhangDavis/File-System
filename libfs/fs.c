#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define FILE_SIG "ECS150FS\0"
#define BLOCK_SIZE 4096
#define FAT_EOC 0xFFFF

/* TODO: Phase 1 */
typedef struct __attribute__((__packed__)) superBlock {
	int8_t signature[8];	
	int16_t totBlocks;	//Total amount of blocks of virtual disk
	int16_t rootBlock;	//Index of root directory block
	int16_t startBlock;	//Index of first data block
	int16_t numDataBlocks;	//Amount of data blocks
	int8_t numFatBlocks;	//Number of FAT blocks
	int8_t padding[4079];	
} *Super;

typedef struct __attribute__((__packed__)) rootDirectory {
	int8_t fileName[16];
	int32_t size;
	int16_t firstIndex;	//Index of first data block
	int8_t padding[10];
} *Root;

Super super_block;
Root root_dir;
uint16_t* Fat;

int fs_mount(const char *diskname)
{
	super_block = (Super)malloc(sizeof(struct superBlock));
	root_dir = (Root)malloc(sizeof(int8_t) * BLOCK_SIZE);

	if (block_disk_open(diskname) == -1)
		return -1;
	
	if (block_read(0, super_block) == -1)
		return -1;

	char sig[20];
	strcpy(sig, (char*)super_block->signature);
	sig[strlen(sig) - 3] = '\0';

	if (strcmp(sig, FILE_SIG) != 0)
	{
		printf("%s, %s\n", sig, FILE_SIG);
		return -1;
	}

	if (block_disk_count() != super_block->totBlocks)
	{
		printf("%d, %d\n", block_disk_count(), super_block->totBlocks);
		return -1; 
	}

	if (block_read(super_block->rootBlock, root_dir) == -1)
		return -1;

	Fat = (uint16_t*)malloc(sizeof(uint16_t) * BLOCK_SIZE * super_block->numFatBlocks);
	int i = 0;
	for (i = 0; i < super_block->numFatBlocks; ++i)
	{
		if (block_read(i + 1, Fat + (BLOCK_SIZE * i)) == -1)
			return -1;
	}

	return 0;
}

int fs_umount(void)
{
	if (block_write(0, super_block) == -1)
		return -1;

	if (block_write(super_block->rootBlock, root_dir) == -1)
		return -1;

	int i = 0;
	for (i = 0; i < super_block->numFatBlocks; ++i)
	{
		if (block_write(i + 1, Fat + (BLOCK_SIZE * i)) == -1)
			return -1;
	}

	free(super_block);
	free(root_dir);
	free(Fat);

	if (block_disk_close() == -1)
		return -1;

	return 0;
}

int fs_info(void)
{
	if(super_block == NULL)
		return -1;

	int i = 0;
	int usedFat = super_block->numDataBlocks;
	for (i = 0; i < super_block->numDataBlocks; ++i)
	{
		if (Fat[i] != 0)
			usedFat--;
	}

	i = 0;
	int freeFiles = 128;
	for (i = 0; i < 128; ++i)
	{
		if(root_dir[i].size != 0)
			freeFiles--;
	}

	printf("FS Info:\n");
	printf("total_blk_count=%d\n", super_block->totBlocks);
	printf("fat_blk_count=%d\n", super_block->numFatBlocks);
	printf("rdir_blk=%d\n", super_block->rootBlock);
	printf("data_blk=%d\n", super_block->startBlock);
	printf("data_blk_count=%d\n", super_block->numDataBlocks);
	printf("fat_free_ratio=%d/%d\n", usedFat, super_block->numDataBlocks);
	printf("rdir_free_ratio=%d/128\n", freeFiles);

	return 0;
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
	return 0;
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
	return 0;
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
	return 0;
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
	return 0;
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
	return 0;
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
	return 0;
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
	return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
	return 0;
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
	return 0;
}

