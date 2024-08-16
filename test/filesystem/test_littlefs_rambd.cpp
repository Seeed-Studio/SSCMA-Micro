#include <gtest/gtest.h>

#include <cstdint>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

#include "bd/lfs_rambd.h"
#include "lfs.h"

namespace ma {

static std::mutex _lfs_mutex;

extern "C" {

static int _lfs_lock(const struct lfs_config* c) {
    _lfs_mutex.lock();
    return 0;
}

static int _lfs_unlock(const struct lfs_config* c) {
    _lfs_mutex.unlock();
    return 0;
}

}

TEST(FILESYSTEM, LittleFSRamBD) {
    const int block_size  = 4096;
    const int block_count = 1024;

    std::vector<uint8_t> ram_buffer(block_size * block_count);

    lfs_rambd_t rambd;

    lfs_rambd_config rambd_cfg = {
      .read_size   = 16,
      .prog_size   = 16,
      .erase_size  = block_size,
      .erase_count = block_count,
      .buffer      = ram_buffer.data(),
    };

    lfs_config cfg = {
      .context          = &rambd,
      .read             = lfs_rambd_read,
      .prog             = lfs_rambd_prog,
      .erase            = lfs_rambd_erase,
      .sync             = lfs_rambd_sync,
#ifdef LFS_THREADSAFE
      .lock             = _lfs_lock,
      .unlock           = _lfs_unlock,
#endif
      .read_size        = rambd_cfg.read_size,
      .prog_size        = rambd_cfg.prog_size,
      .block_size       = block_size,
      .block_count      = block_count,
      .block_cycles     = 500,
      .cache_size       = 16,
      .lookahead_size   = 16,
      .read_buffer      = nullptr,
      .prog_buffer      = nullptr,
      .lookahead_buffer = nullptr,
      .name_max         = 255,
    };

    int ret = lfs_rambd_create(&cfg, &rambd_cfg);
    ASSERT_EQ(ret, LFS_ERR_OK);

    lfs_t lfs;
    ret = lfs_mount(&lfs, &cfg);
    if (ret != LFS_ERR_OK) {
        ret = lfs_format(&lfs, &cfg);
    }
    ASSERT_EQ(ret, LFS_ERR_OK);

    ret = lfs_mount(&lfs, &cfg);
    ASSERT_EQ(ret, LFS_ERR_OK);

    ret = lfs_mkdir(&lfs, "/mydir");
    ASSERT_EQ(ret, LFS_ERR_OK);


    ret = lfs_mkdir(&lfs, "/mydir/subdir");
    ASSERT_EQ(ret, LFS_ERR_OK);

    lfs_file_t file;

    ret = lfs_file_open(&lfs, &file, "/mydir/subdir/file", LFS_O_RDWR | LFS_O_CREAT);
    ASSERT_EQ(ret, LFS_ERR_OK);

    std::string data = "Hello, World!";
    auto        size = lfs_file_write(&lfs, &file, data.data(), data.size());
    ASSERT_EQ(size, data.size());

    ret = lfs_file_close(&lfs, &file);
    ASSERT_EQ(ret, LFS_ERR_OK);

    ret = lfs_file_open(&lfs, &file, "/mydir/subdir/file", LFS_O_RDONLY);
    ASSERT_EQ(ret, LFS_ERR_OK);

    std::vector<uint8_t> buffer(data.size(), 0);
    size = lfs_file_read(&lfs, &file, buffer.data(), buffer.size());
    ASSERT_EQ(size, data.size());

    std::string read_data(buffer.begin(), buffer.end());
    ASSERT_EQ(data, read_data);

    ret = lfs_file_close(&lfs, &file);
    ASSERT_EQ(ret, LFS_ERR_OK);

    ret = lfs_file_open(&lfs, &file, "/mydir/subdir/file", LFS_O_RDONLY);
    ASSERT_EQ(ret, LFS_ERR_OK);

    std::vector<uint8_t> buffer2(128, 0);
    size = lfs_file_read(&lfs, &file, buffer2.data(), buffer2.size());
    ASSERT_EQ(size, buffer.size());

    std::string read_data2(reinterpret_cast<const char*>(buffer2.data()), size);
    ASSERT_EQ(data, read_data2);

    size = lfs_file_read(&lfs, &file, buffer2.data(), buffer2.size());
    ASSERT_EQ(size, 0);

    ret = lfs_file_close(&lfs, &file);
    ASSERT_EQ(ret, LFS_ERR_OK);

    ret = lfs_mkdir(&lfs, "/mydir/subdir2");
    ASSERT_EQ(ret, LFS_ERR_OK);

    ret = lfs_mkdir(&lfs, "/mydir/subdir2");
    ASSERT_EQ(ret, LFS_ERR_EXIST);

    ret = lfs_remove(&lfs, "/mydir/subdir2");
    ASSERT_EQ(ret, LFS_ERR_OK);

    ret = lfs_remove(&lfs, "/mydir/subdir2");
    ASSERT_EQ(ret, LFS_ERR_NOENT);

    ret = lfs_remove(&lfs, "/mydir/subdir");
    ASSERT_EQ(ret, LFS_ERR_NOTEMPTY);

    ret = lfs_remove(&lfs, "/mydir/subdir/file");
    ASSERT_EQ(ret, LFS_ERR_OK);

    ret = lfs_remove(&lfs, "/mydir/subdir/file");
    ASSERT_EQ(ret, LFS_ERR_NOENT);

    ret = lfs_unmount(&lfs);
    ASSERT_EQ(ret, LFS_ERR_OK);

    ret = lfs_rambd_destroy(&cfg);
    ASSERT_EQ(ret, LFS_ERR_OK);
}

}  // namespace ma
