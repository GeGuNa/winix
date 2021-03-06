#ifndef _FS_CONST_HEAD_
#define _FS_CONST_HEAD_

#include <sys/types.h>
#include <fs/type.h>
#include <sys/macro.h>

#define ROOT_INODE_NUM  1

#define BLOCK_SIZE_WORD  (128)
#define DISK_PAGE_NR    (400)

#ifdef __x86_64__
#define SECTOR_SIZE 512
#define BLOCK_SIZE  512
#define CHAR_TO_WORD(x) (x / 4)
#define WORD_TO_CHAR(x) (x * 4)
#else
#define SECTOR_SIZE 128
#define BLOCK_SIZE  128
#define CHAR_TO_WORD(x) (x)
#define WORD_TO_CHAR(x) (x)
#endif

#define INODE_NUM   496
#define NR_TZONES   40

#define NR_FILPS          32    /* # slots in filp table */
#define NR_INODES         48    /* # slots in "in core" inode table */
#define NR_SUPERS          8    /* # slots in super block table */
#define NR_LOCKS           8    /* # slots in the file locking table */

#define READING 1
#define WRITING 2

#ifndef PRIVATE
#define PRIVATE static
#endif

#define VERIFY_READ     1
#define VERIFY_WRITE    2
#define ROOT_DEV    (0x0101)    /* MAKEDEV(1,1) */

#define SUPER_BLOCK_MAGIC   (0xabcdefff)


#define DISK_SIZE   (DISK_PAGE_NR * BLOCK_SIZE)
#define DISK_SIZE_WORD   (DISK_PAGE_NR * BLOCK_SIZE_WORD)


extern unsigned int start_unix_time;
#define get_unix_time()         (start_unix_time + (get_uptime() / HZ))

extern char _DISK_RAW[DISK_SIZE];

#endif

