#define FUSE_USE_VERSION 31

#include "FileOperation.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <cassert>
#include <dirent.h>
#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <cstdio>
#include <pwd.h>
#include <unistd.h>
#include <iostream>
#include "op.h"
#include "Protocol.h"

#define DEBUG_FO

FileOperation::FileOperation() {
    printf("File System init complete\n");
}

FileOperation::~FileOperation() {
    printf("File System shut down\n");
}

void *FileOperation::Init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    (void) conn;
    cfg->use_ino = 1;

    /* Pick up changes from lower filesystem right away. This is
	   also necessary for better hardlink support. When the kernel
	   calls the unlink() handler, it does not know the inode of
	   the to-be-removed entry and can therefore not invalidate
	   the cache of the associated inode - resulting in an
	   incorrect st_nlink value being reported for any remaining
	   hardlinks to this inode. */
    cfg->entry_timeout = 0;
    cfg->attr_timeout = 0;
    cfg->negative_timeout = 0;

    printf("data path: %s\n", get_data_dir());

    // TODO: check failure
    if (mkdir(get_data_dir(), 0777) == -1) {
        if (errno == EEXIST) {
            // already exists
            printf("found previous data\n");
        } else {
            printf("cannot create data folder\n");
        }
    }

    return nullptr;
}

//-------------------init-------------------------------------------------------

int FileOperation::Read(const char *path, char *buf, size_t size, 
                    off_t offset, struct fuse_file_info *fi) {
    std::cerr<<"[debug] Read, path="<<path<<std::endl;
    int fd;
    int res;
    printf("[debug] read\n");
    CONVERT_PATH(real_path, path);
    if (fi == nullptr)
        fd = open(real_path, O_RDONLY);
    else
        fd = fi->fh;

    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    if (fi == nullptr)
        close(fd);

    return res;
}

int FileOperation::Getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    //std::cerr<<"[debug] Getattr, path="<<path<<std::endl;
    (void) fi;
    int res;
    CONVERT_PATH(real_path, path)
    res = lstat(real_path, stbuf);
    if (res == -1)
        return -errno;

    return 0;
}

//---------------------will do something-----------------------------------------------------------------

int FileOperation::Write(const char *path, const char *buf, size_t size, 
            off_t offset, struct fuse_file_info *fi) {
#ifdef DEBUG_FO
    std::cerr<<"[debug] Write, path="<<path<<std::endl;
#endif
    int fd;
    int res;
    printf("[debug] write\n");
    CONVERT_PATH(real_path, path);

    (void) fi;
    if (fi == nullptr)
        fd = open(real_path, O_WRONLY);
    else
        fd = fi->fh;

    if (fd == -1)
        return -errno;

    res = pwrite(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    if (fi == nullptr)
        close(fd);

    return res;
}

int FileOperation::Rename(const char *from, const char *to, unsigned int flags) {
#ifdef DEBUG_FO
    std::cerr<<"[debug] Rename, from="<<from<<", to="<<to<<std::endl;
#endif
    int res;
    CONVERT_PATH(real_from, from)
    CONVERT_PATH(real_to, to)
    if (flags)
        return -EINVAL;

    res = rename(real_from, real_to);
    if (res == -1)
        return -errno;

    return 0;
}

int FileOperation::Open(const char *path, struct fuse_file_info *fi) {
#ifdef DEBUG_FO
    std::cerr<<"[debug] Open, path="<<path<<std::endl;
#endif
    int res;
    printf("[debug] open\n");
    CONVERT_PATH(real_path, path);
    res = open(real_path, fi->flags);
    if (res == -1)
        return -errno;

    fi->fh = res;

    return 0;
}

int FileOperation::Readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi,
                          enum fuse_readdir_flags flags) {
#ifdef DEBUG_FO
    std::cerr<<"[debug] Readdir, path="<<path<<std::endl;
#endif
    DIR *dp;
    struct dirent *de;

    (void) offset;
    (void) fi;
    (void) flags;

    CONVERT_PATH(real_path, path);

    dp = opendir(real_path);
    if (dp == nullptr)
        return -errno;

    while ((de = readdir(dp)) != nullptr) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0, (fuse_fill_dir_flags) 0))
            break;
    }

    closedir(dp);

    return 0;
}

int FileOperation::Create(const char *path, mode_t mode, struct fuse_file_info *fi) {
#ifdef DEBUG_FO
    std::cerr<<"[debug] Create, path="<<path<<std::endl;
#endif
    int res;
    CONVERT_PATH(real_path, path);

    res = open(real_path, fi->flags, mode);
    if (res == -1)
        return -errno;

    fi->fh = res;

    return 0;
}

int FileOperation::Mkdir(const char *path, mode_t mode) {
#ifdef DEBUG_FO
    std::cerr<<"[debug] Mkdir, path="<<path<<std::endl;
#endif
    int res;
    CONVERT_PATH(real_path, path)

    res = mkdir(real_path, mode);
    if (res == -1)
        return -errno;

    return 0;
}

int FileOperation::Rmdir(const char *path) {
#ifdef DEBUG_FO
    std::cerr<<"[debug] Rmdir, path="<<path<<std::endl;
#endif
    int res;
    CONVERT_PATH(real_path, path)

    res = rmdir(real_path);
    if (res == -1)
        return -errno;

    return 0;
}

int FileOperation::Symlink(const char *from, const char *to) {
#ifdef DEBUG_FO
    std::cerr<<"[debug] Symlink, from="<<from<<", to="<<to<<std::endl;
#endif
    int res;
    CONVERT_PATH(real_from, from)
    CONVERT_PATH(real_to, to)

    res = symlink(real_from, real_to);
    if (res == -1)
        return -errno;

    return 0;
}

int FileOperation::Chmod(const char *path, mode_t mode, struct fuse_file_info *fi) {
#ifdef DEBUG_FO
    std::cerr<<"[debug] Chmod, path="<<path<<", mode="<<mode<<std::endl;
#endif
    (void) fi;
    int res;
    CONVERT_PATH(real_path, path);

    res = chmod(real_path, mode);
    if (res == -1)
        return -errno;
    return 0;
}

int FileOperation::Chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi) {
#ifdef DEBUG_FO
    std::cerr<<"[debug] Chown, path="<<path<<", uid="<<uid<<", gid="<<gid<<std::endl;
#endif
    (void) fi;
    int res;
    CONVERT_PATH(real_path, path);
    res = lchown(real_path, uid, gid);
    if (res == -1)
        return -errno;
    return 0;
}

int FileOperation::Readlink(const char *path, char *buf, size_t size) {
#ifdef DEBUG_FO
    std::cerr<<"[debug] Readlink, path="<<path<<std::endl;
#endif
    int res;
    CONVERT_PATH(real_path, path)

    res = readlink(real_path, buf, size - 1);
    if (res == -1)
        return -errno;

    buf[res] = '\0';
    return 0;
}