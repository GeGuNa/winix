#include "fs.h"
#include <winix/list.h>

void init_fs_struct() {
    init_buf();
    init_inode();
    init_filp();
}

