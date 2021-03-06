#pragma once

#include "DriveAgent.h"
#include "SharedQueue.h"
#include "Protocol.h"
#include <mutex>

class DriveServerAgent : public DriveAgent {

private:
    struct BackgroundTask {
        DriveServerAgent *host;

        explicit BackgroundTask(DriveServerAgent *driveServerAgent);

        void run();

        std::thread *stageChangeThread = nullptr;
        bool running = false;

        void stageChanges();

        SharedQueue<OperationRecord> unstagedChanges;

        void addJournal(OperationRecord &r);
    };

    BackgroundTask *backgroundTask;
    uint64_t server_stamp{};
    std::mutex stamp_mutex;

    uint64_t set_server_stamp_as_now();

    uint64_t set_server_stamp_as(uint64_t newStamp);

public:
    uint64_t getServerStamp() const;

    void handlePull(int sockfd, uint64_t last_sync);


    DriveServerAgent(const char *address, int port);

    ~DriveServerAgent() override;

    void handleUpdate(int fd, const std::vector<std::string> &newFiles,
                      const std::vector<std::string> &deleteFiles, const std::vector<std::string> &newDirs,
                      const std::vector<std::string> &deleteDirs,
                      const std::vector<std::pair<std::string, std::string>> &renameDirs);

    int Write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) override;

    int Rename(const char *from, const char *to, unsigned int flags) override;

    int Open(const char *path, struct fuse_file_info *fi) override;

    int Mkdir(const char *path, mode_t mode) override;

    int Unlink(const char *path) override;

    int Rmdir(const char *path) override;

    int Symlink(const char *from, const char *to) override;

    int Chmod(const char *path, mode_t mode, struct fuse_file_info *fi) override;

    int Chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi) override;

    int Readlink(const char *path, char *buf, size_t size) override;

    int Readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi,
                enum fuse_readdir_flags flags) override;

    int Create(const char *path, mode_t mode, struct fuse_file_info *fi) override;

    void *Init(struct fuse_conn_info *conn,
               struct fuse_config *cfg) override;

    int Getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) override;

    int Read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) override;


    uint64_t getLatestStamp();

    std::vector<OperationRecord> readJournal(const std::string &path);
};

