/*
 * Copyright (c) 2023 Seeed Technology Co.,Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *    http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _EL_EXTFS_WE2_H_
#define _EL_EXTFS_WE2_H_

#include "porting/el_extfs.h"

namespace edgelab {

class ExtfsWE2;

class FileWE2 final : public File {
   public:
    ~FileWE2() override;

    void   close() override;
    Status write(const uint8_t*, size_t, size_t*) override;
    Status read(uint8_t*, size_t, size_t*) override;

    friend class ExtfsWE2;

   protected:
    FileWE2(const void*);

   protected:
    void* _file;
};

class ExtfsWE2 final : public Extfs {
   public:
    ExtfsWE2();
    ~ExtfsWE2() override;

    Status     mount(const char*) override;
    void       unmount() override;
    bool       exists(const char*) override;
    bool       isdir(const char*) override;
    Status     mkdir(const char*) override;
    FileStatus open(const char*, int) override;
    Status     cd(const char*) override;

   private:
    static int _fs_mount_times;
    void*      _fs;
};

}  // namespace edgelab

#endif