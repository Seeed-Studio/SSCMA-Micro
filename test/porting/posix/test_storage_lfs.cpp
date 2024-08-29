#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

#include "porting/posix/ma_storage_lfs.h"

namespace ma {

class StorageLfsTest : public ::testing::Test {
   protected:
    void SetUp() override { storage = new ma::StorageLfs("test_data.lfs"); }

    void TearDown() override { std::remove("test_data.lfs"); }

    ma::StorageLfs* storage;
};

TEST_F(StorageLfsTest, SetAndGetInt) {
    int32_t int_value = 42;
    EXPECT_EQ(storage->set("int_key", &int_value, sizeof(int_value)), MA_OK);
    int32_t retrieved_int_value;
    MA_STORAGE_GET_POD(storage, "int_key", retrieved_int_value, 0);
    EXPECT_EQ(retrieved_int_value, int_value);
}

TEST_F(StorageLfsTest, SetAndGetDouble) {
    double double_value = 3.14;
    EXPECT_EQ(storage->set("double_key", &double_value, sizeof(double_value)), MA_OK);
    double retrieved_double_value;
    MA_STORAGE_GET_POD(storage, "double_key", retrieved_double_value, 0);
    EXPECT_DOUBLE_EQ(retrieved_double_value, double_value);
}

TEST_F(StorageLfsTest, SetAndGetString) {
    std::string str_value = "hello";
    EXPECT_EQ(storage->set("string_key", str_value), MA_OK);
    std::string retrieved_str_value;
    EXPECT_EQ(storage->get("string_key", retrieved_str_value), MA_OK);
    EXPECT_EQ(retrieved_str_value, str_value);
}

TEST_F(StorageLfsTest, SetAndGetCString) {
    const char* cstr_value = "hello";
    EXPECT_EQ(storage->set("cstr_key", cstr_value), MA_OK);
    char retrieved_cstr_value[6];
    MA_STORAGE_GET_CSTR(storage, "cstr_key", retrieved_cstr_value, std::strlen(cstr_value), "");
    EXPECT_STREQ(retrieved_cstr_value, cstr_value);
}

TEST_F(StorageLfsTest, ChangeValue) {
    int32_t int_value = 96;
    EXPECT_EQ(storage->set("int_key", &int_value, sizeof(int_value)), MA_OK);
    int32_t retrieved_int_value;
    MA_STORAGE_GET_POD(storage, "int_key", retrieved_int_value, 0);
    EXPECT_EQ(retrieved_int_value, int_value);
    double double_value = 2.71;
    EXPECT_EQ(storage->set("double_key", &double_value, sizeof(double_value)), MA_OK);
    double retrieved_double_value;
    MA_STORAGE_GET_POD(storage, "double_key", retrieved_double_value, 0);
    EXPECT_DOUBLE_EQ(retrieved_double_value, double_value);
}

TEST_F(StorageLfsTest, RemoveKey) {
    MA_STORAGE_NOSTA_SET_RVPOD(storage, "key_to_remove", 123);
    EXPECT_EQ(storage->exists("key_to_remove"), true);
    EXPECT_EQ(storage->remove("key_to_remove"), MA_OK);
    int32_t value;
    EXPECT_EQ(storage->exists("key_to_remove"), false);
}

TEST_F(StorageLfsTest, LongKVFuzz) {
    {
        std::string lk = "abcdefghijklmnopqrstuvwxyz!@#$%^&*()_+1234567890///|{}:<>:'?~``";
        int32_t     v  = 42;
        EXPECT_EQ(storage->set(lk, &v, sizeof(v)), MA_OK);
        int32_t rv;
        MA_STORAGE_GET_POD(storage, lk, rv, 0);
        EXPECT_EQ(rv, v);
    }

    {
        std::string lk = "abcdefghijklmno//////////////////////d/w/e/d/w/e/d/w/ed/w/e/fw/ef/"
                         "regowbgpibpibfibpib2ipirhbp2i3hrpih2pi3hr28889wefg9324g80fg023g008(&%*&%*^%&$%#^$%#^$&"
                         "&^*%^&%$#$*^%*^(&^&^$%^#$^%^pqrstuvwxyz!@#$%^&*()_+1234567890///|{}:<>:'?~``";
        int32_t     v  = 42;
        EXPECT_EQ(storage->set(lk, &v, sizeof(v)), MA_OK);
        int32_t rv;
        MA_STORAGE_GET_POD(storage, lk, rv, 0);
        EXPECT_EQ(rv, v);
    }

    {
        std::string lkv = "abcdefghijklmno//////////////////////d/w/e/d/w/e/d/w/ed/w/e/fw/ef/"
                          "regowbgpibpibfibpib2ipirhbp2i3hrpih2pi3hr28889wefg9324g80fg023g008(&%*&%*^%&$%#^$%#^$&"
                          "&^*%^&%$#$*^%*^(&^&^$%^#$^%^pqrstuvwxyz!@#$%^&*()_+1234567890///|{}:<>:'?~``";
        EXPECT_EQ(storage->set(lkv, lkv), MA_OK);
        std::string rkv;
        EXPECT_EQ(storage->get(lkv, rkv), MA_OK);
        EXPECT_EQ(lkv, rkv);
    }

    {
        std::string lkv = "abcdefghijklmno//////////////////////d/w/e/d/w/e/d/w/ed/w/e/fw/ef/"
                          "regowbgpibpibfibpib2ipirhbp2i3hrpih2pi3hr28889wefg9324g80fg023g008(&%*&%*^%&$%#^$%#^$&"
                          "&^*%^&%$#$*^%*^(&^&^$%^#$^%^pqrstuvwxyz!@#$%^&*()_+1234567890///|{}:<>:'?~``";
        EXPECT_EQ(storage->set(lkv, lkv), MA_OK);
        std::string rkv;
        EXPECT_EQ(storage->get(lkv, rkv), MA_OK);
        EXPECT_EQ(lkv, rkv);

        int32_t v2 = 42;
        EXPECT_EQ(storage->set(lkv, &v2, sizeof(v2)), MA_OK);
        int32_t rv2;
        MA_STORAGE_GET_POD(storage, lkv, rv2, 0);
        EXPECT_EQ(rv2, v2);

        EXPECT_EQ(storage->exists(lkv), true);

        EXPECT_EQ(storage->remove(lkv), MA_OK);

        EXPECT_EQ(storage->remove(lkv), MA_OK);

        EXPECT_EQ(storage->exists(lkv), false);
    }
}

TEST_F(StorageLfsTest, LargeValueFuzz) {
    std::string k = "key/hoihwdi*/(&*(^&%*$))/()(&*^&%$#%$)";

    {
        std::string v(1024 * 1024, 'a');
        EXPECT_EQ(storage->set(k, v), MA_OK);
        std::string rv;
        EXPECT_EQ(storage->get(k, rv), MA_OK);
        EXPECT_EQ(rv, v);
    }

    {
        std::string v2(1024 * 1024, '\0');
        EXPECT_EQ(storage->set(k, v2), MA_OK);
        std::string rv2;
        EXPECT_EQ(storage->get(k, rv2), MA_OK);
        EXPECT_EQ(rv2, v2);
    }

    {
        std::string v3;
        EXPECT_EQ(storage->set(k, v3), MA_OK);
        std::string rv3;
        EXPECT_EQ(storage->get(k, rv3), MA_OK);
        EXPECT_EQ(rv3, v3);
    }

    { EXPECT_EQ(storage->exists(k), true); }

    {
        std::string                     random_bytes;
        std::random_device              rd;
        std::mt19937                    gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (size_t i = 0; i < 1024 * 1024; i++) {
            random_bytes.push_back(static_cast<char>(dis(gen)));
        }
        EXPECT_EQ(storage->set(k, random_bytes), MA_OK);
        std::string random_bytes_read;
        EXPECT_EQ(storage->get(k, random_bytes_read), MA_OK);
        EXPECT_EQ(random_bytes, random_bytes_read);
    }
}

TEST_F(StorageLfsTest, EmptyKey) {
    std::string k;
    std::string v = "value";
    EXPECT_EQ(storage->set(k, v), MA_OK);
    std::string rv;
    EXPECT_EQ(storage->get(k, rv), MA_OK);
    EXPECT_EQ(rv, v);
}

}  // namespace ma
