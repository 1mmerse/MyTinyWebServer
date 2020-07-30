//
// Created by 杨成进 on 2020/7/27.
//

#ifndef MYTINYWEBSERVER_HTTPRESPONSE_H
#define MYTINYWEBSERVER_HTTPRESPONSE_H

#include <unordered_map>
#include <sys/stat.h> //struct stat
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h> //mmap
#include "../buffer/Buffer.h"
#include "../log/Log.h"

class HttpResponse {
public:
    HttpResponse();

    ~HttpResponse();

    void Init(const std::string &srcDir, std::string &path, bool isKeepAlive = false, int code = -1);

    void MakeResponse(Buffer &buff);

    void UnmapFile();

    char *File();

    size_t Filelen() const;

    void ErrorContent(Buffer &buff, std::string message);

    int Code() const;

private:
    void AddStateLine_(Buffer &buff);

    void AddHeader_(Buffer &buff);

    void AddContent_(Buffer &buff);

    void ErrorHtml();

    std::string GetFileType();

    int code_;
    bool isKeepAlive_;

    std::string path_;
    std::string srcDir_;

    char *mmFile_;
//    struct stat {
//        dev_t     st_dev;     /* ID of device containing file */
//        ino_t     st_ino;     /* inode number */
//        mode_t    st_mode;    /* protection */
//        nlink_t   st_nlink;   /* number of hard links */
//        uid_t     st_uid;     /* user ID of owner */
//        gid_t     st_gid;     /* group ID of owner */
//        dev_t     st_rdev;    /* device ID (if special file) */
//        off_t     st_size;    /* total size, in bytes */
//        blksize_t st_blksize; /* blocksize for file system I/O */
//        blkcnt_t  st_blocks;  /* number of 512B blocks allocated */
//        time_t    st_atime;   /* time of last access */
//        time_t    st_mtime;   /* time of last modification */
//        time_t    st_ctime;   /* time of last status change */
//    };
    struct stat mmFileStat_;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
};


#endif //MYTINYWEBSERVER_HTTPRESPONSE_H
