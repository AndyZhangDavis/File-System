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
#define DIV_ROUND_UP(n, d)    (((n) + (d) - 1) / (d))

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
	uint16_t firstIndex;	//Index of first data block
	int8_t padding[10];
} *Root;

typedef struct fd {
	struct rootDirectory fileDescript;
	int offset;
} *Fd;

Super super_block;
Root root_dir;
Fd open_files;
uint16_t* Fat;
int num_open_files;

int get_valid_fd(int fd)
{
	if (fd < 0 || fd > FS_OPEN_MAX_COUNT)
		return 0;

	if (open_files[fd].fileDescript.firstIndex == 0)
		return 0;

	return 1;
}

int find_empty_fat()
{
	int i = 0;

	for (i = 0; i < super_block->numDataBlocks; ++i)
	{
		if (Fat[i] == 0)
			break;
	}
	return i;
}

int fs_mount(const char *diskname)
{
	super_block = (Super)malloc(sizeof(struct superBlock));
	root_dir = (Root)malloc(sizeof(int8_t) * BLOCK_SIZE);
	open_files = (Fd)malloc(sizeof(struct fd) * FS_OPEN_MAX_COUNT);

	if (block_disk_open(diskname) == -1)
		return -1;
	
	if (block_read(0, super_block) == -1)
		return -1;

	char sig[20];
	strcpy(sig, (char*)super_block->signature);
	sig[8] = '\0';

	if (strcmp(sig, FILE_SIG) != 0)
	{
		printf("%s, %s\n", sig, FILE_SIG);
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
		/* TODO potentially, might be wrong */
		if(root_dir[i].firstIndex != 0)
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
	int i = 0;

	if (filename == NULL || strlen(filename) > FS_FILENAME_LEN)
		return -1;

	for (i = 0; i < FS_FILE_MAX_COUNT; ++i)
	{
		if(strcmp((char*)root_dir[i].fileName, filename) == 0)
			return -1;
	}

	for (i = 0; i < FS_FILE_MAX_COUNT; ++i)
	{
		/* TODO potentially, might be wrong */
		if (root_dir[i].firstIndex == 0)
		{
			memset(&(root_dir[i]), 0, FS_OPEN_MAX_COUNT);
			strcpy((char*)root_dir[i].fileName, filename);
			root_dir[i].size = 0;
			root_dir[i].firstIndex = FAT_EOC;
			break;
		}
	}

	if (i == FS_FILE_MAX_COUNT)
		return -1;

	return 0;
}

int fs_delete(const char *filename)
{
	if (filename == NULL)
		return -1;

	int fileIndex = 0;
	for (fileIndex = 0; fileIndex < FS_OPEN_MAX_COUNT; ++fileIndex)
	{
		if (strcmp((char*)open_files[fileIndex].fileDescript.fileName, filename) == 0)
			return -1;
	}

	int i = 0;
	for (i = 0; i < FS_FILE_MAX_COUNT; ++i)
	{
		if(strcmp((char*)root_dir[i].fileName, filename) == 0)
			break;
	}

	if (i == FS_FILE_MAX_COUNT)
		return -1;

	uint16_t index = root_dir[i].firstIndex, tempIndex;
	
	while (index != FAT_EOC)
	{
		tempIndex = Fat[index];
		Fat[index] = 0;
		index = tempIndex;
	}

	root_dir[i].fileName[0] = '\0';
	memset(&(root_dir[i]), 0, FS_OPEN_MAX_COUNT);

	return 0;
}

int fs_ls(void)
{
	if (super_block == NULL)
		return -1;

	printf("FS Ls:\n");

	int i = 0;
	for (i = 0; i < FS_FILE_MAX_COUNT; ++i)
	{
		if(root_dir[i].fileName[0] != '\0')
		{
			printf("file: %s, ", (char*)root_dir[i].fileName);
			printf("size: %d, ", root_dir[i].size);
			printf("data_blk: %u\n", root_dir[i].firstIndex);
		}
	}

	return 0;
}

int fs_open(const char *filename)
{
	if (filename == NULL)
		return -1;

	int fileIndex = 0;
	for (fileIndex = 0; fileIndex < FS_FILE_MAX_COUNT; ++fileIndex)
	{
		if(strcmp((char*)root_dir[fileIndex].fileName, filename) == 0)
			break;
	}

	if (fileIndex == FS_FILE_MAX_COUNT)
		return -1;

	if (num_open_files >= FS_OPEN_MAX_COUNT)
		return -1;

	int openIndex = 0;
	for (openIndex = 0; openIndex < FS_OPEN_MAX_COUNT; ++openIndex)
	{
		if (open_files[openIndex].fileDescript.firstIndex == 0)
		{
			open_files[openIndex].fileDescript = root_dir[fileIndex];
			open_files[openIndex].offset = 0;
			break;
		}
	}

	num_open_files++;

	/* TODO: Phase 3 */
	return openIndex;
}

int fs_close(int fd)
{
	if (!get_valid_fd(fd))
		return -1;

	memset(&(open_files[fd]), 0, sizeof(struct fd));

	num_open_files--;
	
	/* TODO: Phase 3 */
	return 0;
}

int fs_stat(int fd)
{
	if (!get_valid_fd(fd))
		return -1;

	int size = open_files[fd].fileDescript.size;

	/* TODO: Phase 3 */
	return size;
}

int fs_lseek(int fd, size_t offset)
{
	if (!get_valid_fd(fd))
		return -1;

	if (offset > open_files[fd].fileDescript.size)
		return -1;

	open_files[fd].offset = offset;

	/* TODO: Phase 3 */
	return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
	if (!get_valid_fd(fd))
		return -1;

	int numBlocks = DIV_ROUND_UP(count, BLOCK_SIZE);	

	char* bounce = (char*)malloc(BLOCK_SIZE * sizeof(char) * numBlocks);
	size_t blockIndex = open_files[fd].fileDescript.firstIndex;
	int blockOffset = open_files[fd].offset;
	
	if (blockIndex == 0xFFFF)
	{
		blockIndex = find_empty_fat();
		open_files[fd].fileDescript.firstIndex = blockIndex;
		Fat[blockIndex] = FAT_EOC;
	}

	else
	{
		while (blockOffset > BLOCK_SIZE)
		{
			blockIndex = Fat[blockIndex];
			blockOffset -= BLOCK_SIZE;
		}
	}

	blockIndex += super_block->startBlock;

	memcpy(bounce + blockOffset, buf, count);

	block_write(blockIndex, bounce);

	open_files[fd].fileDescript.size = count;
	open_files[fd].offset;

	/* TODO: Phase 4 */
	return count;
}

int fs_read(int fd, void *buf, size_t count)
{
	if (!get_valid_fd(fd))
		return -1;

	int numBlocks = DIV_ROUND_UP(count, BLOCK_SIZE);

	char* bounce = (char*)malloc(BLOCK_SIZE * sizeof(char) * numBlocks);
	int blockIndex = open_files[fd].fileDescript.firstIndex;
	int blockOffset = open_files[fd].offset;

	while (blockOffset > BLOCK_SIZE)
	{
		blockIndex = Fat[blockIndex];
		blockOffset -= BLOCK_SIZE; 
	}
	
	blockIndex += super_block->startBlock;
	block_read(blockIndex, bounce);

	int i = 0;
	for (i = 1; i < numBlocks; ++i)
	{
		printf("Hi\n");
		blockIndex = Fat[blockIndex];
		block_read(blockIndex + super_block->startBlock, bounce + BLOCK_SIZE * i);
	}

	memcpy(buf, bounce + blockOffset, count);

	/* TODO: Phase 4 */	
	return strlen(buf);
}

