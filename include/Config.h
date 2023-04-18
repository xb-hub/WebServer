//
// Created by 许斌 on 2022/9/6.
//

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <iostream>
#include <yaml-cpp/yaml.h>
#include <boost/lexical_cast.hpp>
#include <set>
#include <map>
#include <vector>
#include <list>
#include <functional>
#include <mutex>
#include <thread>
#include <shared_mutex>

namespace xb
{

    template <typename Source, typename Target>
    class LexicalCast
    {
    public:
        Target operator()(const Source &source)
        {
            return boost::lexical_cast<Target>(source);
        }
    };

    template <class T>
    class LexicalCast<std::string, std::vector<T>>
    {
    public:
        std::vector<T> operator()(const std::string &source)
        {
            YAML::Node node = YAML::Load(source);
            std::vector<T> config_list;
            if (node.IsSequence())
            {
                std::stringstream ss;
                for (const auto &item : node)
                {
                    ss.str("");
                    ss << item;
                    config_list.push_back(LexicalCast<std::string, T>()(ss.str()));
                }
            }
            else
            {
                std::cout << "source is not a Sequence" << std::endl;
            }
            return config_list;
        }
    };

    template <class T>
    class LexicalCast<std::vector<T>, std::string>
    {
    public:
        std::string operator()(std::vector<T> &source)
        {
            YAML::Node node;
            for (const auto &item : source)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(item)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <class T>
    class LexicalCast<std::string, std::list<T>>
    {
    public:
        std::list<T> operator()(const std::string &source)
        {
            YAML::Node node = YAML::Load(source);
            std::list<T> config_list;
            if (node.IsSequence())
            {
                std::stringstream ss;
                for (const auto &item : node)
                {
                    ss.str("");
                    ss << item;
                    config_list.push_back(LexicalCast<std::string, T>()(ss.str()));
                }
            }
            else
            {
            }
            return config_list;
        }
    };

    template <class T>
    class LexicalCast<std::list<T>, std::string>
    {
    public:
        std::string operator()(std::list<T> &source)
        {
            YAML::Node node;
            for (const auto &item : source)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(item)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <class T>
    class LexicalCast<std::string, std::set<T>>
    {
    public:
        std::set<T> operator()(const std::string &source)
        {
            YAML::Node node = YAML::Load(source);
            std::set<T> config_list;
            if (node.IsSequence())
            {
                std::stringstream ss;
                for (const auto &item : node)
                {
                    ss.str("");
                    ss << node;
                    config_list.insert(LexicalCast<std::string, T>()(ss.str()));
                }
            }
            else
            {
            }
            return config_list;
        }
    };

    template <class T>
    class LexicalCast<std::set<T>, std::string>
    {
    public:
        std::string operator()(std::set<T> &source)
        {
            YAML::Node node;
            for (const auto &item : source)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(item)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <class T>
    class LexicalCast<std::string, std::map<std::string, T>>
    {
    public:
        std::map<std::string, T> operator()(const std::string &source)
        {
            YAML::Node node = YAML::Load(source);
            std::map<std::string, T> config_list;
            if (node.IsMap())
            {
                std::stringstream ss;
                for (const auto &item : node)
                {
                    ss.str("");
                    ss << item.second;
                    config_list.insert(std::make_pair(item.first.as<std::string>(),
                                                      LexicalCast<std::string, T>()(ss.str())));
                }
            }
            else
            {
            }
            return config_list;
        }
    };

    template <class T>
    class LexicalCast<std::map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::map<std::string, T> &source)
        {
            YAML::Node node(YAML::NodeType::Map);
            for (auto &item : source)
            {
                node[item.first] = YAML::Load(LexicalCast<T, std::string>()(item.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    class ConfigVarBase
    {
    public:
        using ptr = std::shared_ptr<ConfigVarBase>;
        ConfigVarBase(const std::string &name, const std::string &description) : name_(name),
                                                                                 description_(description)
        {
            std::transform(name_.begin(), name_.end(), name_.begin(), ::tolower);
        }

        std::string getName() const { return name_; }
        std::string getDescription() const { return description_; }

        virtual std::string toString() = 0;
        virtual bool fromString(const std::string &str) = 0;

    private:
        std::string name_;
        std::string description_;
    };

    template <class T>
    class ConfigVar : public ConfigVarBase
    {
    public:
        using ptr = std::shared_ptr<ConfigVar>;
        using modifyCallback = std::function<void(const T &, const T &)>;
        ConfigVar(const std::string &name, const std::string &description, const T &value) : ConfigVarBase(name, description),
                                                                                             value_(value)
        {
        }
        ~ConfigVar() = default;
        T getValue() const { return value_; }
        void setValue(const T &value)
        {
            {
                std::shared_lock<std::shared_mutex> lock(rw_lock_);
                if (value == value_)
                    return;
                T old_value = value_;
                for (const auto &it : callback_map_)
                {
                    it.second(old_value, value);
                }
            }
            std::scoped_lock<std::shared_mutex> lockGraud(rw_lock_);
            value_ = value;
        }

        std::string toString() override
        {
            try
            {
                return LexicalCast<T, std::string>()(getValue());
            }
            catch (const std::exception &e)
            {
                std::cerr << "ConfigVar::toString exception"
                          << e.what()
                          << " convert: "
                          << "string to "
                          << typeid(value_).name() << std::endl;
            }
            return "error!";
        }
        bool fromString(const std::string &str) override
        {
            try
            {
                setValue(LexicalCast<std::string, T>()(str));
                return true;
            }
            catch (const std::exception &e)
            {
                std::cerr << "ConfigVar::fromString exception"
                          << e.what()
                          << " convert: "
                          << "string to "
                          << typeid(value_).name() << std::endl;
            }
            return false;
        }

        uint64_t addListener(modifyCallback cb)
        {
            static uint64_t index = 0;
            std::scoped_lock<std::shared_mutex> lock(rw_lock_);
            callback_map_[++index] = cb;
            return index;
        }

        void delListener(uint64_t key)
        {
            std::scoped_lock<std::shared_mutex> lock(rw_lock_);
            callback_map_.erase(key);
        }

        modifyCallback getListener(uint64_t key)
        {
            std::shared_lock<std::shared_mutex> lock(rw_lock_);
            auto iter = callback_map_.find(key);
            if (iter == callback_map_.end())
                return nullptr;
            return iter->second;
        }

        void clearListener()
        {
            std::scoped_lock<std::shared_mutex> lock(rw_lock_);
            callback_map_.clear();
        }

    private:
        T value_;
        std::map<uint64_t, modifyCallback> callback_map_;
        std::shared_mutex rw_lock_;
    };

    class Config
    {
    public:
        using ptr = std::shared_ptr<Config>;
        using ConfigVarMap = std::map<std::string, ConfigVarBase::ptr>;
        Config() = default;
        ~Config() = default;

        static typename ConfigVarBase::ptr LookUp(const std::string &name)
        {
            ConfigVarMap &data = getData();
            auto iter = data.find(name);
            if (iter == data.end())
                return nullptr;
            return iter->second;
        }

        template <class T>
        static typename ConfigVar<T>::ptr LookUp(const std::string &name)
        {
            auto iter = LookUp(name);
            if (!iter)
                return nullptr;
            auto ptr = std::dynamic_pointer_cast<ConfigVar<T>>(iter);
            return ptr;
        }

        template <class T>
        static typename ConfigVar<T>::ptr LookUp(const std::string &name, const T &value, const std::string &description = "")
        {
            ConfigVarMap &data = getData();
            auto iter = LookUp<T>(name);
            if (iter)
                return iter;
            if (name.find_first_not_of("qwertyuiopasdfghjklzxcvbnm0123456789._") != std::string::npos)
            {
                std::cerr << "Config::LookUp exception" << std::endl;
                throw std::invalid_argument(name);
            }
            auto tmp = std::make_shared<ConfigVar<T>>(name, description, value);
            std::scoped_lock<std::shared_mutex> lock(getRWLock());
            data[name] = tmp;
            return tmp;
        }

        static void LoadFromYaml(const YAML::Node &node)
        {
            std::vector<std::pair<std::string, YAML::Node>> nodes_list;
            TravelAllNode(node, "", nodes_list);
            for (const auto &node : nodes_list)
            {
                std::string key = node.first;
                if (key.empty())
                {
                    continue;
                }
                std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                auto var = LookUp(key);
                if (var)
                {
                    std::stringstream ss;
                    ss << node.second;
                    var->fromString(ss.str());
                }
            }
        }

        static void Visit(std::function<void(ConfigVarBase::ptr)> cb)
        {
            std::shared_lock<std::shared_mutex> lock(getRWLock());
            ConfigVarMap &m = getData();
            for (auto it = m.begin();
                 it != m.end(); ++it)
            {
                cb(it->second);
            }
        }

    private:
        static void TravelAllNode(const YAML::Node &node, const std::string name,
                                  std::vector<std::pair<std::string, YAML::Node>> &node_list)
        {
            auto iter = std::find_if(node_list.begin(), node_list.end(),
                                     [&name](const std::pair<std::string, YAML::Node> &item)
                                     { return item.first == name; });
            if (iter != node_list.end())
                iter->second = node;
            else
                node_list.push_back(std::make_pair(name, node));
            if (node.IsSequence())
            {
                for (size_t i = 0; i < node.size(); i++)
                {
                    TravelAllNode(node[i], name + "." + std::to_string(i), node_list);
                }
            }
            else if (node.IsMap())
            {
                for (auto &it : node)
                {
                    TravelAllNode(it.second, name.empty() ? it.first.Scalar() : name + "." + it.first.Scalar(), node_list);
                }
            }
        }

    private:
        static ConfigVarMap &getData()
        {
            static ConfigVarMap data;
            return data;
        }

        static std::shared_mutex &getRWLock()
        {
            static std::shared_mutex rw_lock;
            return rw_lock;
        }
    };

}

#endif //_CONFIG_H_
