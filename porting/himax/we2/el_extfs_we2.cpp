#include "el_extfs_we2.h"

#include <WE2_core.h>
#include <WE2_device.h>
#include <ff.h>
#include <hx_drv_gpio.h>
#include <hx_drv_scu.h>

#include <cstring>

#include "porting/el_misc.h"

#define CONFIG_CURDIR_NAME_MAX_LEN 256

extern "C" {
void SSPI_CS_GPIO_Output_Level(bool setLevelHigh) { hx_drv_gpio_set_out_value(GPIO16, (GPIO_OUT_LEVEL_E)setLevelHigh); }

void SSPI_CS_GPIO_Pinmux(bool setGpioFn) {
    if (setGpioFn)
        hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_GPIO16, 0);
    else
        hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_SPI_M_CS_1, 0);
}

void SSPI_CS_GPIO_Dir(bool setDirOut) {
    if (setDirOut)
        hx_drv_gpio_set_output(GPIO16, GPIO_OUT_HIGH);
    else
        hx_drv_gpio_set_input(GPIO16);
}
}

namespace edgelab {

static Status STATUS_FROM_FRESULT(FRESULT res) {
    switch (res) {
    case FR_OK:
        return {true, ""};
    case FR_DISK_ERR:
        return {false, "A hard error occurred in the low level disk I/O layer"};
    case FR_INT_ERR:
        return {false, "Assertion failed"};
    case FR_NOT_READY:
        return {false, "The physical drive cannot work"};
    case FR_NO_FILE:
        return {false, "Could not find the file"};
    case FR_NO_PATH:
        return {false, "Could not find the path"};
    case FR_INVALID_NAME:
        return {false, "The path name format is invalid"};
    case FR_DENIED:
        return {false, "Access denied due to prohibited access or directory full"};
    case FR_EXIST:
        return {false, "Access denied due to prohibited access"};
    case FR_INVALID_OBJECT:
        return {false, "The file/directory object is invalid"};
    case FR_WRITE_PROTECTED:
        return {false, "The physical drive is write protected"};
    case FR_INVALID_DRIVE:
        return {false, "The logical drive number is invalid"};
    case FR_NOT_ENABLED:
        return {false, "The volume has no work area"};
    case FR_NO_FILESYSTEM:
        return {false, "There is no valid FAT volume"};
    case FR_MKFS_ABORTED:
        return {false, "The f_mkfs() aborted due to any parameter error"};
    case FR_TIMEOUT:
        return {false, "Could not get a grant to access the volume within defined period"};
    case FR_LOCKED:
        return {false, "The operation is rejected according to the file sharing policy"};
    case FR_NOT_ENOUGH_CORE:
        return {false, "LFN working buffer could not be allocated"};
    case FR_TOO_MANY_OPEN_FILES:
        return {false, "Number of open files > _FS_SHARE"};
    case FR_INVALID_PARAMETER:
        return {false, "Given parameter is invalid"};
    default:
        return {false, "Unknown error"};
    }
}

FileWE2::~FileWE2() { close(); }

void FileWE2::close() {
    f_close(static_cast<FIL*>(_file));
    delete static_cast<FIL*>(_file);
    _file = nullptr;
}

Status FileWE2::write(const uint8_t* data, size_t size, size_t* written) {
    UINT    bw;
    FRESULT res = f_write(static_cast<FIL*>(_file), data, size, &bw);
    if (written) {
        *written = bw;
    }
    return STATUS_FROM_FRESULT(res);
}

Status FileWE2::read(uint8_t* data, size_t size, size_t* read) {
    UINT    br;
    FRESULT res = f_read(static_cast<FIL*>(_file), data, size, &br);
    if (read) {
        *read = br;
    }
    return STATUS_FROM_FRESULT(res);
}

FileWE2::FileWE2(const void* f) {
    _file                     = new FIL{};
    *static_cast<FIL*>(_file) = *static_cast<const FIL*>(f);
}

ExtfsWE2::ExtfsWE2() { _fs = new FATFS{}; }

ExtfsWE2::~ExtfsWE2() {
    unmount();

    delete static_cast<FATFS*>(_fs);
    _fs = nullptr;
}

Status ExtfsWE2::mount(const char* path) {
    hx_drv_scu_set_PB2_pinmux(SCU_PB2_PINMUX_SPI_M_DO_1, 1);
    hx_drv_scu_set_PB3_pinmux(SCU_PB3_PINMUX_SPI_M_DI_1, 1);
    hx_drv_scu_set_PB4_pinmux(SCU_PB4_PINMUX_SPI_M_SCLK_1, 1);
    hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_SPI_M_CS_1, 1);

    char* curdir = nullptr;

    FRESULT res = f_mount(static_cast<FATFS*>(_fs), "", 1);
    if (res != FR_OK) {
        goto MountReturn;
    }

    curdir = new char[CONFIG_CURDIR_NAME_MAX_LEN]{};
    if (curdir == nullptr) {
        res = FR_NOT_ENOUGH_CORE;
        goto MountReturn;
    }

    res = f_getcwd(curdir, CONFIG_CURDIR_NAME_MAX_LEN);
    if (res != FR_OK) {
        goto MountReturn;
    }

    {
        const char* dir = path;
        char*       drv = std::strchr(curdir, ':');
        if (drv != nullptr) {
            dir = drv + 1;
        }
        if (strcmp(dir, path) != 0) {
            size_t len = CONFIG_CURDIR_NAME_MAX_LEN - (drv - curdir);
            if (len < strlen(path)) {
                res = FR_INVALID_NAME;
                goto MountReturn;
            }
            if (snprintf(drv, len, "%s", path) < 0) {
                res = FR_INVALID_NAME;
                goto MountReturn;
            }
            res = f_chdir(curdir);
        }
    }

MountReturn:
    if (curdir != nullptr) {
        delete[] curdir;
        curdir = nullptr;
    }

    return STATUS_FROM_FRESULT(res);
}

void ExtfsWE2::unmount() { f_unmount(""); }

bool ExtfsWE2::exists(const char* f) {
    FILINFO fno;
    FRESULT res = f_stat(f, &fno);
    return res == FR_OK && std::strlen(fno.fname) > 0;
}

bool ExtfsWE2::isdir(const char* f) {
    FILINFO fno;
    FRESULT res = f_stat(f, &fno);
    return res == FR_OK && fno.fattrib & AM_DIR;
}

Status ExtfsWE2::mkdir(const char* path) {
    FRESULT res = f_mkdir(path);
    return STATUS_FROM_FRESULT(res);
}

FileStatus ExtfsWE2::open(const char* path, int mode) {
    FIL  file;
    BYTE m = 0;

    if (mode & OpenMode::READ) {
        m |= FA_OPEN_EXISTING | FA_READ;
    }
    if (mode & OpenMode::WRITE) {
        m |= FA_CREATE_ALWAYS | FA_WRITE;
    }
    if (mode & OpenMode::APPEND) {
        m |= FA_OPEN_APPEND | FA_WRITE;
    }

    FRESULT res = f_open(&file, path, m);
    if (res != FR_OK) {
        return {nullptr, STATUS_FROM_FRESULT(res)};
    }

    return {std::unique_ptr<FileWE2>(new FileWE2(&file)), STATUS_FROM_FRESULT(res)};
}

Extfs* Extfs::get_ptr() {
    static ExtfsWE2 extfs{};
    return &extfs;
}

}  // namespace edgelab