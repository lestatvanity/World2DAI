
#pragma once

#include <boost/asio.hpp>
#include <thread>
#include <unordered_map>
#include <memory>
#include "ScriptManager.h"

class Session;

class Server {
public:
    Server(boost::asio::io_context& io_context, short port);
    void start();
    void loadWorld(const std::string& path);
private:
    void doAccept();

    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::io_context& io_context_;

    std::unordered_map<std::string, std::shared_ptr<Session>> sessions_;
    ScriptManager scriptManager_;
};
