
#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <array>
#include "ScriptManager.h"

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::ip::tcp::socket socket, ScriptManager& scriptMgr);

    void start();

private:
    void doRead();
    void handleCommand(const std::string& msg);

    boost::asio::ip::tcp::socket socket_;
    std::array<char, 1024> buffer_;
    ScriptManager& scriptManager_;
    std::string playerID_;
};
