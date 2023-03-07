//
// Created by 许斌 on 2022/9/7.
//

#include <yaml-cpp/yaml.h>
#include <iostream>
#include <fstream>
using namespace std;

void print_yaml(YAML::Node config, int level)
{
    if (config.IsSequence())
    {
        for (size_t i = 0; i < config.size(); i++)
        {
            cout << config[i] << endl;
            print_yaml(config[i], level++);
        }
    }
    else if (config.IsMap())
    {
        for (auto it : config)
        {
            cout << it.first << ":";
            print_yaml(it.second, level + 1);
        }
    }
    else if (config.IsScalar())
    {
        cout << config.Scalar() << endl;
    }
}

int main()
{
    YAML::Node config = YAML::LoadFile("../config.yaml");
    cout << config["logs"][1]["formatter"].as<string>() << endl;
}