#include "Server.h"
#include "NPC.h"
#include "Entity.h"
#include "Session.h"
#include "ScriptManager.h"
#include <fstream>       // ← per std::ifstream e std::getline
#include <sstream>       // ← per std::istringstream
#include <iostream>

Server::Server(boost::asio::io_context& io_context, short port)
    : io_context_(io_context),
      acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      scriptManager_("scripts") {
}

void Server::start() {
    std::cout << "[Server] In ascolto sulla porta " << acceptor_.local_endpoint().port() << "\n";
    doAccept();
}

void Server::doAccept() {
    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io_context_);
    acceptor_.async_accept(*socket, [this, socket](boost::system::error_code ec) {
        if (!ec) {
            std::cout << "[Server] Nuova connessione accettata.\n";
            auto session = std::make_shared<Session>(std::move(*socket), scriptManager_);
            session->start();
        }
        doAccept(); // continua ad accettare
    });
}

#include <nlohmann/json.hpp>
using json = nlohmann::json;

void Server::loadWorld(const std::string& path) {
    std::ifstream mapFile(path);
    if (!mapFile.is_open()) {
        std::cerr << "[Server] Impossibile aprire mappa: " << path << "\n";
        return;
    }

    std::ifstream jsonFile("assets/entities.json");
    if (!jsonFile.is_open()) {
        std::cerr << "[Server] Mancante: assets/entities.json\n";
        return;
    }

    json entityData;
    jsonFile >> entityData;

    int y = 0;
    std::string line;
    while (std::getline(mapFile, line)) {
        std::istringstream iss(line);
        int tile, x = 0;

        while (iss >> tile) {
            if (tile == 42) {
                std::string id = "npc_" + std::to_string(x) + "_" + std::to_string(y);
                NPC* npc = new NPC(id, x * 32, y * 32);

                if (entityData.contains(id)) {
                    std::string script = entityData[id];
                    scriptManager_.bindScriptToEntity(npc, script);
                    std::cout << "[Server] Spawned " << id << " con script " << script << "\n";
                }
                else {
                    std::cout << "[Server] Spawned " << id << " (nessuno script associato)\n";
                }
            }
            x++;
        }
        y++;
    }

    std::cout << "[Server] Mappa caricata con NPC dinamici\n";
}


