//
// Created by bruce on 17/05/20.
//

#include "fs.h"

#define DIRECT_BLOCK_IO 

const char* DEVICE_NAME = "sda";
const char* FS_TYPE = "wfs";

static char* raw_disk = NULL;
static size_t raw_disk_size = 0;

static char* rootfs_disk;
static size_t rootfs_disk_size;

struct device rootfs_dev;
struct superblock root_sb;
//static struct file_system root_fs;
dev_t devid = ROOT_DEV;

struct superblock* get_sb(struct device* id){
    if(id == &rootfs_dev)
        return &root_sb;
    return NULL;
}

void set_raw_disk(char *disk, size_t size){
    raw_disk = disk;
    raw_disk_size = size;
}

int blk_dev_io_read(char *buf, off_t off, size_t len){
    char *ptr;
    int count = len;
    if(off >= rootfs_disk_size)
        return 0;

    if(off + len > rootfs_disk_size){
        count = rootfs_disk_size - off;
        len = count;
    }

    // KDEBUG(("dev read blk %d %d\n", off / root_sb.s_block_size, len));
    ptr = rootfs_disk + off;
    while(count-- > 0){
        *buf++ = *ptr++;
    }
    return len;
}

int blk_dev_io_write(char *buf, off_t off, size_t len){
    char *ptr;
    int count = len;
    if(off >= rootfs_disk_size)
        return 0;

    if(off + len > rootfs_disk_size){
        count = rootfs_disk_size - off;
        len = count;
    }
    // KDEBUG(("dev write blk %d %d\n", off / root_sb.s_block_size, len));
    ptr = rootfs_disk + off;
    while(count-- > 0){
        *ptr++ = *buf++;
    }
    return len;
}

void kdebug_superblock(){
    KDEBUG(("s_block_inuse: %d | s_inode_inuse: %d | s_free_blocks: %d | s_free_inodes: %d |\n s_block_size: %d | s_inode_size: %d | s_inode_limit: %d | s_rootnr: %d |\n s_superblock_nr: %d | s_superblock_size: %d | s_blockmapnr: %d |  s_blockmap_size: %d |\n s_inodemapnr: %d |  s_inodemap_size: %d | s_inode_tablenr: %d | s_inode_table_size: %d |\n s_inode_per_block %d\n", 
        root_sb.s_block_inuse, root_sb.s_inode_inuse, root_sb.s_free_blocks, root_sb.s_free_inodes, root_sb.s_block_size, root_sb.s_inode_size, root_sb.s_inode_limit, root_sb.s_rootnr, root_sb.s_superblock_nr, root_sb.s_superblock_size, root_sb.s_blockmapnr, root_sb. s_blockmap_size, root_sb.s_inodemapnr, root_sb. s_inodemap_size, root_sb.s_inode_tablenr, root_sb.s_inode_table_size, root_sb.s_inode_per_block));
}

int blk_dev_init(){
    int i;
#ifdef __wramp__
    if(raw_disk == NULL){
        raw_disk = _DISK_RAW;
        raw_disk_size = DISK_SIZE;
    }
#endif
    rootfs_disk = raw_disk;
    rootfs_disk_size = raw_disk_size;
    memcpy(&root_sb, rootfs_disk, sizeof(struct superblock));
    arch_superblock(&root_sb);
    ASSERT(root_sb.magic == SUPER_BLOCK_MAGIC);
    ASSERT(root_sb.s_block_size == BLOCK_SIZE);
 
    #ifdef __wramp__
    kdebug_superblock();
    KDEBUG(("\nsuperblock: block in use %d inode table size %d\n", root_sb.s_block_inuse, root_sb.s_inode_table_size));
    #endif
    return 0;
}

void arch_superblock(struct superblock* sb){
#ifdef __wramp__
    sb->s_inode_size /= 4;
    sb->s_block_size /= 4;
    sb->s_superblock_size /= 4;
    sb->s_blockmap_size /= 4;
    sb->s_inodemap_size /= 4;
    sb->s_inode_table_size /= 4;
#endif
}

void dearch_superblock(struct superblock* sb){
#ifdef __wramp__
    sb->s_inode_size *= 4;
    sb->s_block_size *= 4;
    sb->s_superblock_size *= 4;
    sb->s_blockmap_size *= 4;
    sb->s_inodemap_size *= 4;
    sb->s_inode_table_size *= 4;
#endif
}

int blk_dev_release(){
    return 0;
}

#ifdef DIRECT_BLOCK_IO

static int init_block(struct block_buffer *buf){
    return 0;
}

static int retrieve_block(struct block_buffer *buf, struct device *dev, block_t bnr){
    struct superblock* sb = get_sb(dev);
    int blksize = sb->s_block_size;
    off_t off = bnr * blksize;
    buf->block = rootfs_disk + off;
    buf->b_size = blksize;
    return blksize;
}

static int flush_block(struct block_buffer *buf){
    return buf->b_size;
}

static int release_block(struct block_buffer *buf){
    buf->block = NULL;
    return 0;
}

static struct block_operations bops = {init_block, retrieve_block, flush_block, release_block};

#else

static int buffered_init_block(struct block_buffer *buf){
    buf->block = (char*)get_free_page(GFP_HIGH);
    return 0;
}

static int buffered_retrieve_block(struct block_buffer *buf, struct device *dev, block_t bnr){
    struct superblock* sb = get_sb(dev);
    int blksize = sb->s_block_size;
    off_t off = bnr * blksize;
    buf->b_size = blksize;
    return blk_dev_io_read(buf->block, off, blksize);
}

static int buffered_flush_block(struct block_buffer *buf){
    off_t off = buf->b_blocknr * buf->b_size;
    return blk_dev_io_write(buf->block, off, buf->b_size);
}

static int buffered_release_block(struct block_buffer *buf){
    release_pages((ptr_t *)buf->block, 1);
    buf->block = NULL;
    return 0;
}
static struct block_operations bops = {buffered_init_block, buffered_retrieve_block, buffered_flush_block, buffered_release_block};


#endif

int root_fs_read (struct filp *filp, char *data, size_t count, off_t offset){
    int ret = 0, r, j, blksize;
    unsigned int len;
    off_t off, end_in_block;
    unsigned int curr_fp_index, fp_limit;
    block_t bnr;
    inode_t *ino = NULL;
    struct block_buffer* buffer = NULL;

    blksize = filp->filp_ino->i_sb->s_block_size;
    curr_fp_index = offset / blksize;
    off = offset % blksize;
    fp_limit = (filp->filp_pos + count ) / blksize;
    ino = filp->filp_ino;

    for( ; curr_fp_index <= fp_limit; curr_fp_index++){
        bnr = ino->i_zone[curr_fp_index];
        if(bnr == 0)
            continue;

        len = ((blksize - off) > count) ? count : blksize - off;
        len = (off + len) < ino->i_size ? len : ino->i_size - off;
        r = 0;
        if(filp->filp_flags & O_DIRECT){
            off += bnr * blksize;
            r = filp->filp_dev->dops->dev_read(data, off, len);
            data += r;
        }else{
            char *p;
            size_t len2 = len;
            buffer = get_block_buffer(bnr, filp->filp_dev);
            p = &buffer->block[off];
            while(len2-- > 0){
                *data++ = *p++;
            }
            r += (int)len;
            put_block_buffer(buffer);
        }

        count -= len;
        ret += r;
        filp->filp_pos += r;
        // KDEBUG(("file read for block %d, off %d len %d remaining %d\n", bnr, off, r, count));
        if (r == 0)
            break;
        off = 0;
    }
    return ret;
}

// char* get_buffer_data(char* data, size_t count){
//     static char __buffer[50];
//     char* p = __buffer;
//     while(count--){
//         *p++ = *data++;
//     }
//     *p = '\0';
//     return __buffer;
// }

int root_fs_write (struct filp *filp, char *data, size_t count, off_t offset){
    int r, j, ret = 0, blksize;
    unsigned int len;
    off_t off;
    unsigned int curr_fp_index, fp_limit;
    block_t bnr;
    inode_t *ino = NULL;
    struct block_buffer* buffer = NULL;

    blksize = filp->filp_ino->i_sb->s_block_size;
    curr_fp_index = offset / blksize;
    off = offset % blksize;
    fp_limit = (filp->filp_pos + count ) / blksize;
    fp_limit = fp_limit < NR_TZONES ? fp_limit : NR_TZONES;
    ino = filp->filp_ino;

    for( ; curr_fp_index <= fp_limit; curr_fp_index++){
        bnr = ino->i_zone[curr_fp_index];
        if(bnr == 0){
            if((bnr = alloc_block(ino, ino->i_dev)) > 0){
                ino->i_zone[curr_fp_index] = bnr;
            }else{
                return ENOSPC;
            }
        }

        len = ((blksize - off) > count) ? count : blksize - off;
        r = 0;
        if(filp->filp_flags & O_DIRECT){
            off += bnr * blksize;
            r = filp->filp_dev->dops->dev_write(data, off, len);
            data += r;
        } else{
            char *p;
            size_t len2 = len;
            buffer = get_block_buffer(bnr, filp->filp_dev);
            p = &buffer->block[off];
            while(len2-- > 0){
                *p++ = *data++;
            }
            r += (int)len;
            put_block_buffer_dirt(buffer);
        }
        // KDEBUG(("file write for block %d, off %d len %d, size %d\n", curr_fp_index, off, r, filp->filp_ino->i_size + r));
        /* Read or write 'chunk' bytes. */
        if (r == 0)
            break;

        count -= len;
        ret += r;
        filp->filp_pos += r;
        filp->filp_ino->i_size += r;
        off = 0;
    }
    // KDEBUG(("Rootfs %d write count %d, offset %d ret %d data %s\n",filp->filp_ino->i_num, count, offset, ret, get_buffer_data(data, count)));
    return ret;
}

int root_fs_open (struct device* dev, struct filp *file){
    return 0;
}

int root_fs_close (struct device* dev, struct filp *filp){
    filp->filp_count -= 1;
    if(filp->filp_count == 0){
        put_inode(filp->filp_ino, true);
        // KDEBUG(("closing count %d link %d\n", filp->filp_ino->i_count, filp->filp_ino->i_nlinks));
        if(filp->filp_ino->i_count == 0 && filp->filp_ino->i_nlinks == 0){
            release_inode(filp->filp_ino);
        }
    }
    return OK;
}

int root_fs_ioctl(struct filp* file, int request_type, ptr_t* arg){
    return OK;
}

static struct device_operations dops = {blk_dev_init, blk_dev_io_read, blk_dev_io_write, blk_dev_release};
static struct filp_operations ops = {root_fs_open, root_fs_read, root_fs_write, root_fs_close, root_fs_ioctl};

void register_root_fs_driver(){
    rootfs_dev.bops = &bops;
    register_device(&rootfs_dev, DEVICE_NAME, devid, S_IFREG, &dops, &ops);
}

