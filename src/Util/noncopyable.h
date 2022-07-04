#ifndef __SRC_UTIL_NONCOPYABLE_H_
#define __SRC_UTIL_NONCOPYABLE_H_

class noncopyable {
protected:
    noncopyable() = default;
    ~noncopyable() = default;

public:
    noncopyable(const noncopyable &) = delete;
    const noncopyable &operator=(const noncopyable &) = delete;
};

#endif /* __SRC_UTIL_NONCOPYABLE_H_ */
