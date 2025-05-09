#include "Session.h"
#include <iostream>
#include <sstream>

Session::Session(boost::asio::ip::tcp::socket socket, ScriptManager& scriptMgr)
    : socket_(std::move(socket)), scriptManager_(scriptMgr) {
    playerID_ = "player_" + std::to_string(reinterpret_cast<uintptr_t>(this));
}

void Session::start() {
    std::cout << "[Session] Avviata sessione per " << playerID_ << "\n";
    doRead();
}

void Session::doRead() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(buffer_), [this, self](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
            std::string msg(buffer_.data(), length);
            handleCommand(msg);
            doRead(); // continua lettura
        } else {
            std::cerr << "[Session] Disconnessione: " << playerID_ << "\n";
        }
    });
}

void Session::handleCommand(const std::string& msg) {
    std::istringstream iss(msg);
    std::string cmd;
    iss >> cmd;

    if (cmd == "interact") {
        std::string entityID;
        iss >> entityID;
        std::cout << "[Session] " << playerID_ << " ha interagito con " << entityID << "\n";
        scriptManager_.callScriptFunction(entityID, "onInteract");
    } else if (cmd == "say") {
        std::string msgContent;
        std::getline(iss, msgContent);
        std::cout << "[Chat] " << playerID_ << ": " << msgContent << "\n";
    } else {
        std::cout << "[Comando] " << playerID_ << ": " << msg << "\n";
    }
}
