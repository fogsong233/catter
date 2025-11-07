#pragma once

#include <cstddef>
#include <initializer_list>
#include <linux/limits.h>

namespace catter {
class Recorder {
public:
    Recorder() = delete;

    explicit Recorder(const char* dir) noexcept;
    virtual ~Recorder() noexcept;

    virtual int write(std::initializer_list<const char*> data) noexcept;
    virtual int writeErr(const char* data) noexcept;
    virtual int writeCmd(const char* data) noexcept;
    virtual bool valid() noexcept;

protected:
    const char* getFilePath() noexcept;

protected:
    constexpr static size_t dir_sz_ = PATH_MAX;
    char dir_[dir_sz_ + 1];
    char* file_name_index_;
};
}  // namespace catter
