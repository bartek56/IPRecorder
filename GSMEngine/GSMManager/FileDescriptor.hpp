#pragma once

#include <unistd.h>

class FileDescriptor
{
public:
    FileDescriptor() = default;
    explicit FileDescriptor(int descriptor) noexcept : descriptor(descriptor) {}

    FileDescriptor(const FileDescriptor &) = delete;
    FileDescriptor &operator=(const FileDescriptor &) = delete;

    FileDescriptor(FileDescriptor &&other) noexcept : descriptor(other.release()) {}

    FileDescriptor &operator=(FileDescriptor &&other) noexcept
    {
        if(this != &other)
            reset(other.release());
        return *this;
    }

    ~FileDescriptor()
    {
        reset();
    }

    int get() const noexcept
    {
        return descriptor;
    }

    void reset(int newDescriptor = -1) noexcept
    {
        if(descriptor != -1)
            ::close(descriptor);
        descriptor = newDescriptor;
    }

    int release() noexcept
    {
        const int releasedDescriptor = descriptor;
        descriptor = -1;
        return releasedDescriptor;
    }

private:
    int descriptor = -1;
};
