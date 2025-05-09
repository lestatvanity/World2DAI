#pragma once
#include <string>

class GameObject {
public:
    GameObject(const std::string& id);

    void open();
    void close();
    bool isOpened() const;

    std::string getId() const;

private:
    std::string id;
    bool openState;
};
