#include "file_system.h"
#include "systemcall.h"

// file variable
boot_block_t* boot_block; // Pointer to the boot block
inode_t* inode_block;  // Pointer to the inodes
data_block_t* data_block;  // Pointer to the data blocks
dentry_t open_dir; // dentry for the directory read
dentry_t open_file; // dentry for the file read
uint32_t dir_index;

/*
file_system_init
    DESCRIPTION: initialize the file system
    INPUT:
    file_start_addr: the start address of the file system
    OUTPUT: none
    SIDE EFFECT: change the file variable
*/
void file_system_init(uint32_t file_start_addr){
    // init the file range variable: pointer to boot block/ inode block/ data block
    boot_block = (boot_block_t*)file_start_addr;
    inode_block = (inode_t*)(boot_block + 1);
    data_block = (data_block_t*)(inode_block + boot_block->inode_count);
    dir_index = 0;

}

/*
read_dentry_by_name
    DESCRIPTION: read directory entry according to the name 
    INPUT: 
    fname: file name we want to read
    dentry: pointer to the reading dentry
    OUTPUT: return 0 if success and -1 if fail
    SIDE EFFECT: write the dentry
*/
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry){
    // declare the variable we need to use
    int i;
    uint8_t* file_name;
    //sanity check: invalid file name (too long) or NULL pointer
    if (strlen( (int8_t*)fname ) > FILENAME_LEN || fname == NULL){
        printf("invalid argument for read_dentry_by_name\n");
        return -1;
    }

    // loop in the directory entries to find the file
    for (i = 0; i < boot_block->dir_count; i++){
        file_name = boot_block->dir_entries[i].filename;
        if (strncmp((int8_t*)fname, (int8_t*)file_name, FILENAME_LEN) == 0){ // means we find the file that has the same name
            // copy to the dentry: file name; file type; inode number
            strncpy((int8_t*)(dentry->filename), (int8_t*)(fname), FILENAME_LEN);
            dentry->filetype = boot_block->dir_entries[i].filetype;
            dentry->inode_num = boot_block->dir_entries[i].inode_num;
            return 0;
        }
    }
    // loop end, we don't find so the file don't exist, fail
    return -1;
}

/*
read_dentry_by_index
    DESCRIPTION: read directory entry according to the index
    INPUT:
    index: dentry index we want to read
    dentry: pointer to the reading dentry
    OUTPUT: return 0 if success and -1 if fail
    SIDE EFFECT: write the dentry
*/
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry){
    // declare the variable we need to use
    uint8_t* fname;

    //sanity check: invalid index
    if (index >= boot_block->dir_count || index >= FILENUM_MAX || index < 0){
        return -1;
    }

    fname = boot_block->dir_entries[index].filename;
    // copy to the dentry: file name; file type; inode number
    strncpy((int8_t*)(dentry->filename), (int8_t*)(fname), FILENAME_LEN);
    dentry->filetype = boot_block->dir_entries[index].filetype;
    dentry->inode_num = boot_block->dir_entries[index].inode_num;
    return 0;

}

/*
read_data
    DESCRIPTION: read the data of fixed length from the given inode and offset
    INPUT:
    inode: the inode we want to read
    offset: the offset in the file
    buf: get the bytes we read
    length: the length of byte we are going to read
    OUTPUT: the number of bytes we read
    SIDE EFFECT: change data in the buf
*/
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
    // the variable we need to use
    uint32_t i;
    uint32_t byte_counter;
    uint32_t start_index; // data block index we start to read
    uint32_t start_offset; // offset when we start to read
    uint32_t end_index; // data block we end reading
    uint32_t end_offset; // offset when we end reading
 
    uint32_t cur_index; // current data block index
    data_block_t* cur_block; // Point to current data block

    // sanity check
    if(inode >= boot_block->inode_count - 1 || offset > inode_block[inode].len || length == 0){
        return 0;
    }

    if (length + offset > inode_block[inode].len){
        length = inode_block[inode].len - offset;
    }

    byte_counter = 0;

    // get the start (end) index and offset
    start_index = offset / BLOCK_SIZE;
    end_index = (offset + length - 1) / BLOCK_SIZE;
    start_offset = offset % BLOCK_SIZE;
    end_offset = (offset + length - 1) % BLOCK_SIZE;

    // only one data block condition
    if(start_index == end_index){
        // get the index and block we want to read
        cur_index = (inode_block[inode].data_block_num)[start_index];
        cur_block = &(data_block[cur_index]);
        // copy "length" byte
        memcpy(buf, &(cur_block->data[start_offset]), length);
        byte_counter += length;
        return byte_counter;
    }

    for(i = start_index; i <= end_index; i++){
        // get the index and block we want to read
        cur_index = (inode_block[inode].data_block_num)[i];
        cur_block = &(data_block[cur_index]);
        // start block
        if(i == start_index){
            // start from start offset
            memcpy(buf, &(cur_block->data[start_offset]), BLOCK_SIZE - start_offset);
            byte_counter += BLOCK_SIZE-start_offset;
        }
        // end block
        else if(i == end_index){
            memcpy(buf + byte_counter, cur_block->data, end_offset + 1 );
            byte_counter += end_offset + 1;
            return byte_counter;
        }
        // else, all the data (4kB) should be copied
        else {
            memcpy(buf + byte_counter, cur_block->data, BLOCK_SIZE);
            byte_counter += BLOCK_SIZE;
        }    
    }
    return byte_counter;
}

/*
file_open
    DESCRIPTION: open the file
    INPUT:
    file_name: the file name we want to open
    OUTPUT: return 0 if success and -1 if fail
    SIDE EFFECT: none
*/
int32_t file_open (const uint8_t* file_name){
    // the variable we need to use
    int32_t rev;
    dentry_t dentry;
    // open the file
    rev = read_dentry_by_name(file_name,&(dentry));
    return rev;
}

/*
file_close
    DESCRIPTION: close the file
    INPUT: fd(file descriptor)
    OUTPUT: 0 if success (always success)
    SIDE EFFECT: close the file
*/
int32_t file_close (int32_t fd){
    return 0;
}

/*
file_read
    DESCRIPTION: read the file based on the file descriptor
    INPUT:
    fd: file descriptor
    buf: get the byte we read
    nbytes: the lengh of byte we want to read
    OUTPUT: return the number of bytes we read
    SIDE EFFECT: read the files
*/
int32_t file_read (int32_t fd, void* buf, int32_t nbytes){
    // the variable we need to use
    int32_t cur_pid;
    // sanity check
    if (buf == 0) {
        return 0;
    }
    // get the cur_pcb and find the inode, offset in the file_decs[fd]
    int32_t esp;
    asm volatile ("          \n\
                 movl %%esp, %0  \n\
            "
            :"=r"(esp)
            );
    cur_pid = (BAMB - esp) / BAKB;
    pcb_t* cur_pcb = get_pcb(cur_pid);
    // read the data
    return read_data(cur_pcb->file_decs[fd].inode, cur_pcb->file_decs[fd].file_position, (uint8_t *) buf, nbytes);
}

/*
file_write
    DESCRIPTION: write to the file (not used since it is read-only file system)
    INPUT: ignore
    OUTPUT: return -1 for the failure (always fail)
    SIDE EFFECT: none
*/
int32_t file_write (int32_t fd, const void* buf, int32_t nbytes){
    return -1;
}

/*
directory_open
    DESCRIPTION: open the directory
    INPUT:
    file_name: the file name we want to open
    OUTPUT: return 0 if success and -1 if fail
    SIDE EFFECT: none
*/
int32_t directory_open(const uint8_t* file_name){
    // the variable we need to use
    int32_t rev;
    // open the directory
    rev = read_dentry_by_name(file_name, &(open_dir));
    // check whether it is a directory
    // if (open_dir.filetype != 1){
    //     rev = -1;
    // }
    return rev;
}

/*
directory_close
    DESCRIPTION: close the directory
    INPUT: fd(file descriptor)
    OUTPUT: 0 if success (always success)
    SIDE EFFECT: close the file
*/
int32_t directory_close(int32_t fd){
    return 0;
}

/*
directory_read
    DESCRIPTION: read the directory
    INPUT:
    fd: file descriptor
    buf: get the directory we read
    nbytes: the byte we want to read
    OUTPUT: return the number of bytes we read or 0 if fail
    SIDE EFFECT: cahnge the file range variable
*/
int32_t directory_read(int32_t fd, void* buf, int32_t nbytes){
    // the variable we need to use
    int rev;
    dentry_t dir_entry;

    // sanity check
    if (buf == NULL || dir_index >= boot_block->dir_count){
        dir_index = 0; //reset the dir_index
        return 0;
    }

    // get the directory we want and go to next index
    dir_entry = boot_block->dir_entries[dir_index];
    dir_index ++;

    // file_size = (inode_block + dir_entry.inode_num)->len;
    memcpy(buf,&dir_entry.filename,FILENAME_LEN);

    // printf("open file is:");
    // for (i = 0; i < strlen((int8_t*)dir_entry.filename); i++){
    //     printf("%c",dir_entry.filename[i]);
    // }
    // printf("\n");
    // get the length of file name
    rev = strlen((int8_t*)dir_entry.filename);
    if (strlen((int8_t*)dir_entry.filename) >= FILENAME_LEN){
        rev = FILENAME_LEN;
    }
    // dir_index = 0;
    return rev;
}

/*
directory_write
    DESCRIPTION: write to the directory (not used since it is read-only file system)
    INPUT: ignore
    OUTPUT: return -1 for the failure (always fail)
    SIDE EFFECT: none
*/
int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes){
    return -1;
}
