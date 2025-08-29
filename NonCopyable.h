#ifndef _NONCOPYABLE_H
#define _NONCOPYABLE_H

class NonCopyable
{
protected:
    NonCopyable() = default;
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;
};

#endif
