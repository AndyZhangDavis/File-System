#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

/* TODO: Phase 1 */
typedef struct superBlock {
	int8_t signature[8];
	int16_t totBlocks;
	int16_t rootBlock;
	int16_t startBlock;
	int16_t numDataBlocks;
	int8_t numFatBlocks;
	int8_t padding[4079];	
} *Super;

typedef struct fatBlock {
	int16_t blockIndex[2048];
} *Fat;

typedef struct rootDirectory {
	int8_t fileName[16];
	int32_t size;
	int16_t firstIndex;
	int8_t padding[10];
} *Root;

Super super_block;

int fs_mount(const char *diskname)
{
	super_block = malloc(sizeof(struct superBlock));
	if (block_disk_open(diskname) == -1)
		return -1;
	
	if (block_read

	/* TODO: Phase 1 */
	return 0;
}

int fs_umount(void)
{
	/* TODO: Phase 1 */
}

int fs_info(void)
{
	/* TODO: Phase 1 */
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

