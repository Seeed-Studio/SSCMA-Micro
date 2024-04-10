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

#ifndef _EL_EXTFS_H_
#define _EL_EXTFS_H_

#include <cstddef>
#include <cstdint>
#include <memory>

namespace edgelab {

class Extfs;
class File;

struct Status {
    bool        success = false;
    const char* message = "";
};

struct FileStatus {
    std::unique_ptr<File> file;
    Status                status;
};

class File {
   public:
    virtual ~File() = default;

    virtual void   close()                                = 0;
    virtual Status write(const uint8_t*, size_t, size_t*) = 0;

    friend class Extfs;

   protected:
    File() = default;
};

enum OpenMode : int { READ, WRITE, APPEND };

class Extfs {
   public:
    [[nodiscard]] static Extfs* get_ptr();

    virtual ~Extfs() = default;

    virtual Status     mount(const char*)     = 0;
    virtual void       unmount()              = 0;
    virtual bool       exists(const char*)    = 0;
    virtual bool       isdir(const char*)     = 0;
    virtual FileStatus open(const char*, int) = 0;

   protected:
    Extfs() = default;
};

}  // namespace edgelab

#endif
