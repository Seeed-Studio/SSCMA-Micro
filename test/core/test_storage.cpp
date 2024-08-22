#include <gtest/gtest.h>
#include <sscma.h>

class StorageFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        storage = new ma::StorageFile("test_data.ini");
    }

    void TearDown() override {
        std::remove("test_data.ini");
    }

    ma::StorageFile* storage;
};

TEST_F(StorageFileTest, SetAndGetInt) {
    int32_t int_value = 42;
    EXPECT_EQ(storage->set("int_key", int_value), MA_OK);
    int32_t retrieved_int_value;
    EXPECT_EQ(storage->get("int_key", retrieved_int_value), MA_OK);
    EXPECT_EQ(retrieved_int_value, int_value);
}

TEST_F(StorageFileTest, SetAndGetDouble) {
    double double_value = 3.14;
    EXPECT_EQ(storage->set("double_key", double_value), MA_OK);
    double retrieved_double_value;
    EXPECT_EQ(storage->get("double_key", retrieved_double_value), MA_OK);
    EXPECT_DOUBLE_EQ(retrieved_double_value, double_value);
}

TEST_F(StorageFileTest, SetAndGetString) {
    std::string str_value = "hello";
    EXPECT_EQ(storage->set("string_key", str_value), MA_OK);
    std::string retrieved_str_value;
    EXPECT_EQ(storage->get("string_key", retrieved_str_value), MA_OK);
    EXPECT_EQ(retrieved_str_value, str_value);
}

TEST_F(StorageFileTest, ChangeValue) {
    int32_t int_value = 96;
    EXPECT_EQ(storage->set("int_key", int_value), MA_OK);
    int32_t retrieved_int_value;
    EXPECT_EQ(storage->get("int_key", retrieved_int_value), MA_OK);
    EXPECT_EQ(retrieved_int_value, int_value);
    double double_value = 2.71;
    EXPECT_EQ(storage->set("double_key", double_value), MA_OK);
    double retrieved_double_value;
    EXPECT_EQ(storage->get("double_key", retrieved_double_value), MA_OK);
    EXPECT_DOUBLE_EQ(retrieved_double_value, double_value);
}

TEST_F(StorageFileTest, RemoveKey) {
    EXPECT_EQ(storage->set("key_to_remove", 123), MA_OK);
    EXPECT_EQ(storage->remove("key_to_remove"), MA_OK);
    int32_t value;
    EXPECT_EQ(storage->exists("key_to_remove"), false);
}
