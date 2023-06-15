#ifndef ONLYET_SINGLETON_H
#define ONLYET_SINGLETON_H

#define MY_DISABLE_COPY_MOVE(Class)             \
    Class(const Class &) = delete;              \
    Class &operator=(const Class &) = delete;   \
    Class(Class &&) = delete;                   \
    Class &operator=(Class &&) = delete;

namespace onlyet {

template <typename DerivedClass>
struct Singleton {
    static DerivedClass& instance() {
        static DerivedClass s_instance;
        return s_instance;
    }

protected:
    Singleton()  = default;
    ~Singleton() = default;

private:
    MY_DISABLE_COPY_MOVE(Singleton)
};

// demo
#if 0
class GlobalConfiguration : public QObject, public Singleton<GlobalConfiguration>
{
    Q_OBJECT

private:
    friend Singleton<GlobalConfiguration>;
    explicit GlobalConfiguration(QObject* parent = nullptr);
    ~GlobalConfiguration();
};
#endif

}  // namespace onlyet

#endif  // !ONLYET_SINGLETON_H
