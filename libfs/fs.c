#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

/*
 *	Macros
 *
 */

#define FILE_SIG "ECS150FS\0"
#define BLOCK_SIZE 4096
#define FAT_EOC 0xFFFF
#define DIV_ROUND_UP(n, d)    (((n) + (d) - 1) / (d))

/*
 *	Structs
 *
 */

typedef struct __attribute__((__packed__)) superBlock {
	int8_t signature[8];	
	int16_t totBlocks;	//Total amount of blocks of virtual disk
	int16_t rootBlock;	//Index of root directory block
	int16_t startBlock;	//Index of first data block
	int16_t numDataBlocks;	//Amount of data blocks
	int8_t numFatBlocks;	//Number of FAT blocks
	int8_t padding[4079];	
} *Super;

typedef struct __attribute__((__packed__)) file {
	int8_t fileName[16];
	int32_t size;
	uint16_t firstIndex;	//Index of first data block
	int8_t padding[10];
} *Directory;

typedef struct fd {
	Directory fileDescript;
	int offset;
} *Fd;

/*
 *	Global Variables
 *
 */

Super super_block;
Directory root_dir;
Fd open_files;
uint16_t* Fat;
int num_open_files;

/*
 *	Helper Functions
 *
 */

/* Check validity of fd passed in */
int get_valid_fd(int fd)
{
	if (fd < 0 || fd > FS_OPEN_MAX_COUNT)
		return 0;
	
	if (open_files[fd].fileDescript == NULL)
		return 0;

	if (open_files[fd].fileDescript->firstIndex == 0)
		return 0;

	return 1;
}

/* Find and return first empty fat index */ 
int find_empty_fat()
{
	int i = 0;

	/* Loop through Fat until empty one is found */
	for (i = 0; i < super_block->numDataBlocks; ++i)
	{
		if (Fat[i] == 0)
			break;
	}
	return i;
}

/* Initialize needBlocks of necessary fat */
void add_fat_blocks(int currBlocks, int needBlocks, int fatIndex)
{
	/* Determine what Fat index to start with */
	int i = 0;
	for (i = 0; i < currBlocks; ++i)
		fatIndex = Fat[fatIndex];
	
	if (needBlocks - currBlocks > 2)
		needBlocks++;

	/* Add extra fat blocks */
	for (i = currBlocks + 1; i < needBlocks; ++i)
	{
		Fat[fatIndex] = find_empty_fat();
		fatIndex = Fat[fatIndex];		
	}

	/* Set final fat index for file */
	Fat[fatIndex] = FAT_EOC;
}

/*
 *	API Functions
 *
 */

int fs_mount(const char *diskname)
{
	/* Initialize globals */
	super_block = (Super)malloc(sizeof(struct superBlock));
	root_dir = (Directory)malloc(sizeof(int8_t) * BLOCK_SIZE);
	open_files = (Fd)malloc(sizeof(struct fd) * FS_OPEN_MAX_COUNT);

	/* Open disk */
	if (block_disk_open(diskname) == -1)
		return -1;
	
	/* Read super_block from disk */
	if (block_read(0, super_block) == -1)
		return -1;

	/* Make sure disk signature is FILE_SIG */
	char sig[20];
	strcpy(sig, (char*)super_block->signature);
	sig[8] = '\0';

	if (strcmp(sig, FILE_SIG) != 0)
		return -1;

	/* Read root directory from disk */
	if (block_read(super_block->rootBlock, root_dir) == -1)
		return -1;

	/* Read Fat data from disk */
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
	/* Write super_block to disk */
	if (block_write(0, super_block) == -1)
		return -1;

	/* Write root directory to disk */
	if (block_write(super_block->rootBlock, root_dir) == -1)
		return -1;

	/* Write Fat data out to disk */
	int i = 0;
	for (i = 0; i < super_block->numFatBlocks; ++i)
	{
		if (block_write(i + 1, Fat + (BLOCK_SIZE * i)) == -1)
			return -1;
	}

	/* Free variables, close disk */
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

	/* Find number of used fat indices */
	int i = 0;
	int usedFat = super_block->numDataBlocks;
	for (i = 0; i < super_block->numDataBlocks; ++i)
	{
		if (Fat[i] != 0)
			usedFat--;
	}

	/* Find space for files in root directory */
	i = 0;
	int freeFiles = FS_FILE_MAX_COUNT;
	for (i = 0; i < FS_FILE_MAX_COUNT; ++i)
	{
		if(root_dir[i].firstIndex != 0)
			freeFiles--;
	}

	/* Print info about disk */
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

	/* Check if filename is valid */
	if (filename == NULL || strlen(filename) > FS_FILENAME_LEN)
		return -1;

	/* Check if file already exists in root directory */
	for (i = 0; i < FS_FILE_MAX_COUNT; ++i)
	{
		if(strcmp((char*)root_dir[i].fileName, filename) == 0)
			return -1;
	}

	/* Find first open spot in root directory */
	for (i = 0; i < FS_FILE_MAX_COUNT; ++i)
	{
		if (root_dir[i].firstIndex == 0)
		{
			/* Initialize a new empty file */
			memset(&(root_dir[i]), 0, FS_OPEN_MAX_COUNT);
			strcpy((char*)root_dir[i].fileName, filename);
			root_dir[i].size = 0;
			root_dir[i].firstIndex = find_empty_fat();
			Fat[root_dir[i].firstIndex] = FAT_EOC;
			break;
		}
	}

	/* If no open files */
	if (i == FS_FILE_MAX_COUNT)
		return -1;

	return 0;
}

int fs_delete(const char *filename)
{
	if (filename == NULL)
		return -1;

	/* Check if file of filename is in open_files */
	int fileIndex = 0;
	for (fileIndex = 0; fileIndex < FS_OPEN_MAX_COUNT; ++fileIndex)
	{
		/* Prevent dereferencing of NULL pointer */
		if (open_files[fileIndex].fileDescript == NULL)
			continue;
		/* If file is still open */
		else if (strcmp((char*)open_files[fileIndex].fileDescript->fileName, filename) == 0)
			return -1;
	}

	/* Find file in root directory */
	int i = 0;
	for (i = 0; i < FS_FILE_MAX_COUNT; ++i)
	{
		if(strcmp((char*)root_dir[i].fileName, filename) == 0)
			break;
	}

	/* If file wasn't found */
	if (i == FS_FILE_MAX_COUNT)
		return -1;

	uint16_t index = root_dir[i].firstIndex, tempIndex;

	/* Clear Fat blocks containing this file */	
	while (index != FAT_EOC)
	{
		tempIndex = Fat[index];
		Fat[index] = 0;
		index = tempIndex;
	}

	/* Clear file from root directory */
	root_dir[i].fileName[0] = '\0';
	memset(&(root_dir[i]), 0, FS_OPEN_MAX_COUNT);

	return 0;
}

int fs_ls(void)
{
	if (super_block == NULL)
		return -1;

	printf("FS Ls:\n");

	/* Print all current files data */
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

	/* Find file in root directory */
	int fileIndex = 0;
	for (fileIndex = 0; fileIndex < FS_FILE_MAX_COUNT; ++fileIndex)
	{
		if(strcmp((char*)root_dir[fileIndex].fileName, filename) == 0)
			break;
	}

	/* If file wasn't found */
	if (fileIndex == FS_FILE_MAX_COUNT)
		return -1;

	/* Or if max files are currently open */
	if (num_open_files >= FS_OPEN_MAX_COUNT)
		return -1;

	/* Copy data from file in root directory to first open entry in open_files */
	int openIndex = 0;
	for (openIndex = 0; openIndex < FS_OPEN_MAX_COUNT; ++openIndex)
	{
		if (open_files[openIndex].fileDescript == NULL)
		{
			open_files[openIndex].fileDescript = &(root_dir[fileIndex]);
			open_files[openIndex].offset = 0;
			break;
		}
	}

	/* Increment number of open files */
	num_open_files++;

	return openIndex;
}

int fs_close(int fd)
{
	/* Check filedescriptor validity */
	if (!get_valid_fd(fd))
		return -1;

	/* Clear file fd from open files */
	memset(&(open_files[fd]), 0, sizeof(struct fd));

	/* Decrement number of open files */
	num_open_files--;
	
	return 0;
}

int fs_stat(int fd)
{
	/* Check filedescriptor validity */
	if (!get_valid_fd(fd))
		return -1;

	int size = open_files[fd].fileDescript->size;

	return size;
}

int fs_lseek(int fd, size_t offset)
{
	/* Check filedescriptor validity */
	if (!get_valid_fd(fd))
		return -1;

	/* Check bounds of offset */
	if (offset > open_files[fd].fileDescript->size)
		return -1;

	open_files[fd].offset = offset;

	return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
	/* Check filedescriptor validity */
	if (!get_valid_fd(fd))
		return -1;

	/* Skip allocation if file is empty */
	if (count == 0)
		return 0;

	/* Initialize necessary variables */
	int needBlocks = DIV_ROUND_UP(count, BLOCK_SIZE);	
	int currBlocks = DIV_ROUND_UP(open_files[fd].fileDescript->size, BLOCK_SIZE);
	char* prevBlock = (char*)malloc(BLOCK_SIZE * sizeof(char));
	int blockIndex = open_files[fd].fileDescript->firstIndex;
	int blockOffset = open_files[fd].offset;
	int numBytesRemain = count;

	/* Allocate blocks necessary */
	if (open_files[fd].fileDescript->size < count + open_files[fd].offset)
	{
		add_fat_blocks(currBlocks, needBlocks, blockIndex);
		open_files[fd].fileDescript->size = count;
	}

	/* Change write block if offset causes block switch */
	while (blockOffset > BLOCK_SIZE)
	{
		blockIndex = Fat[blockIndex];
		blockOffset -= BLOCK_SIZE;
	}

	/* Read first block to memory */	
	block_read(blockIndex + super_block->startBlock, prevBlock);
	memcpy(prevBlock + blockOffset, buf, BLOCK_SIZE - blockOffset);
	block_write(blockIndex + super_block->startBlock, prevBlock);
	numBytesRemain -= BLOCK_SIZE;
	numBytesRemain += blockOffset;

	/* Move read location from buffer */
	buf += BLOCK_SIZE - blockOffset;
	blockIndex = Fat[blockIndex];

	/* Write middle blocks (entire blocks) if any */
	int i = 0;
	for (i = 0; i < needBlocks - 2; ++i)
	{
		block_write(blockIndex + super_block->startBlock, buf);	
		buf += BLOCK_SIZE;
		blockIndex = Fat[blockIndex];
		numBytesRemain -= BLOCK_SIZE;
	} 

	/* Write final block to memory */
	if (currBlocks + 1 < needBlocks)
	{
		block_read(blockIndex + super_block->startBlock, prevBlock);
		memcpy(prevBlock, buf, numBytesRemain);
		block_write(blockIndex + super_block->startBlock, prevBlock);
	}

	open_files[fd].offset += count;

	return count;
}

int fs_read(int fd, void *buf, size_t count)
{
	/* Check filedescriptor validity */
	if (!get_valid_fd(fd))
		return -1;

	/* Initialize necessary variables */
	int numBlocks = DIV_ROUND_UP(count, BLOCK_SIZE);
	char* bounce = (char*)malloc(BLOCK_SIZE * sizeof(char) * numBlocks);
	int blockIndex = open_files[fd].fileDescript->firstIndex;
	int blockOffset = open_files[fd].offset;

	/* Change read block if offset causes block switch */
	while (blockOffset > BLOCK_SIZE)
	{
		blockIndex = Fat[blockIndex];
		blockOffset -= BLOCK_SIZE; 
	}

	/* Read each block to a bounce buffer */
	int i = 0;
	for (i = 0; i < numBlocks; ++i)
	{
		block_read(blockIndex + super_block->startBlock, bounce + BLOCK_SIZE * i);
		blockIndex = Fat[blockIndex];
	}

	/* Copy bounce buffer to output buffer */
	memcpy(buf, bounce + blockOffset, count);

	open_files[fd].offset += strlen(buf);

	/* Return total bytes read */
	return strlen(buf);
}

