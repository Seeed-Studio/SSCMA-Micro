#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

#include "bd/lfs_filebd.h"
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

TEST(FILESYSTEM, LittleFSFileBD) {
    const int block_size  = 4096;
    const int block_count = 1024;

    const ::testing::TestInfo* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string                file_path = std::filesystem::current_path().string() + "/test_" +
                            std::string(test_info->test_case_name()) + "_" + std::string(test_info->name()) + ".lfs";
    std::cout << "file_path: " << file_path << std::endl;

    lfs_filebd_t      filebd;
    lfs_filebd_config filebd_cfg = {
      .read_size   = 16,
      .prog_size   = 16,
      .erase_size  = block_size,
      .erase_count = block_count,
    };
    lfs_config cfg = {
      .context          = &filebd,
      .read             = lfs_filebd_read,
      .prog             = lfs_filebd_prog,
      .erase            = lfs_filebd_erase,
      .sync             = lfs_filebd_sync,
#ifdef LFS_THREADSAFE
      .lock             = _lfs_lock,
      .unlock           = _lfs_unlock,
#endif
      .read_size        = filebd_cfg.read_size,
      .prog_size        = filebd_cfg.prog_size,
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

    int ret = lfs_filebd_create(&cfg, file_path.c_str(), &filebd_cfg);
    ASSERT_EQ(ret, LFS_ERR_OK);

    lfs_t lfs;
    ret = lfs_mount(&lfs, &cfg);
    if (ret != LFS_ERR_OK) {
        ret = lfs_format(&lfs, &cfg);
    }
    ASSERT_EQ(ret, LFS_ERR_OK);

    ret = lfs_mount(&lfs, &cfg);
    ASSERT_EQ(ret, LFS_ERR_OK);

    // check if dir exists
    lfs_dir_t dir;
    ret = lfs_dir_open(&lfs, &dir, "/mydir");
    if (ret != LFS_ERR_OK) {
        ret = lfs_mkdir(&lfs, "/mydir");
        if (ret == LFS_ERR_EXIST) {
            ret = lfs_remove(&lfs, "/mydir");
            ASSERT_EQ(ret, LFS_ERR_OK);
            ret = lfs_mkdir(&lfs, "/mydir");
        }
        ASSERT_EQ(ret, LFS_ERR_OK);
        ret = lfs_dir_open(&lfs, &dir, "/mydir");
    }
    ASSERT_EQ(ret, LFS_ERR_OK);

    ret = lfs_dir_close(&lfs, &dir);
    ASSERT_EQ(ret, LFS_ERR_OK);

    lfs_file_t file;
    ret = lfs_file_open(&lfs, &file, "/mydir/file", LFS_O_RDWR | LFS_O_CREAT);
    ASSERT_EQ(ret, LFS_ERR_OK);

    std::string data = "Hello, World!";
    auto        size = lfs_file_write(&lfs, &file, data.data(), data.size());
    ASSERT_EQ(size, data.size());

    ret = lfs_file_close(&lfs, &file);
    ASSERT_EQ(ret, LFS_ERR_OK);

    ret = lfs_file_open(&lfs, &file, "/mydir/file", LFS_O_RDONLY);
    ASSERT_EQ(ret, LFS_ERR_OK);

    std::vector<uint8_t> buffer(data.size(), 0);
    size = lfs_file_read(&lfs, &file, buffer.data(), buffer.size());
    ASSERT_EQ(size, data.size());

    std::string read_data(buffer.begin(), buffer.end());
    ASSERT_EQ(data, read_data);

    ret = lfs_file_close(&lfs, &file);
    ASSERT_EQ(ret, LFS_ERR_OK);

    ret = lfs_unmount(&lfs);
    ASSERT_EQ(ret, LFS_ERR_OK);

    ret = lfs_filebd_destroy(&cfg);
    ASSERT_EQ(ret, LFS_ERR_OK);
}

}  // namespace ma
