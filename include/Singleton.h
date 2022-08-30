//
// Created by 许斌 on 2022/5/10.
//

#ifndef SINGLETON_H
#define SINGLETON_H

namespace xb
{
    template<class T>
    class Singleton
    {
    private:
        Singleton() = default;

    public:
        static T *getInstance()
        {
            static T instance;
            return &instance;
        }
    };

    template<typename T>
    class SingletonPtr final
    {
    private:
        SingletonPtr() = default;

    public:
        static std::shared_ptr<T> getInstance()
        {
#ifdef __DEBUG
            std::cout << "singleton" << std::endl;
#endif
            static std::shared_ptr<T> instance(new T);
            return instance;
        }
    };
}

#endif //EPOLL_H_SINGLETON_H
