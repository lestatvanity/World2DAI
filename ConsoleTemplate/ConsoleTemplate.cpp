#define IMPLEMENT_SERVER_MAIN
#include "Game.h"
#include "MenuState.h"
#include "EditorState.h"
#include "Server.h"
#include <windows.h>
#include <csignal>
#include <imgui-SFML.h>
#include <boost/asio.hpp>
#include <iostream>
#include <string>

BOOL WINAPI consoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        std::cout << "\n[INFO] Ricevuto Ctrl+C, uscita sicura...\n";
        // fai cleanup se serve (salva dati, libera risorse, ecc.)
        ExitProcess(0); // chiusura gentile
    }
    return TRUE;
}

int main(int argc, char* argv[]) {

    if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
        std::cerr << "Errore nella registrazione del gestore Ctrl+C\n";
        return 1;
    }

    std::cout << "App in esecuzione. Premi Ctrl+C per uscire.\n";
    if (argc > 1) {
        std::string mode = argv[1];

        if (mode == "-server") {
            std::cout << "[Server] Avvio server...\n";
            try {
                boost::asio::io_context io_context;
                Server server(io_context, 5000);
                server.loadWorld("assets/mappa.txt");
                server.start();

                io_context.run(); // loop principale asincrono
            }
            catch (const std::exception& e) {
                std::cerr << "[Server] Errore: " << e.what() << "\n";
                return 1;
            }

            return 0;
        }

        if (mode == "-editor") {
            //std::cout << "[Editor] Avvio editor locale offline...\n";

            sf::RenderWindow window(sf::VideoMode(1280, 720), "World2DeAi Editor", sf::Style::Default);
            window.setVerticalSyncEnabled(true); // ok
            std::cout << "[main] Finestra editor creata\n";

            ImGui::SFML::Init(window);
            //ImGuiIO& io = ImGui::GetIO();
            //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

            std::cout << "[main] ImGui inizializzato\n";

            Game game;
            game.setWindow(&window); // IMPORTANTE: prima del resto
            game.init();                 // ora possiamo chiamare updateView()

            auto editorState = std::make_unique<EditorState>(game);
            editorState->initFromConfig("assets/world.ini");
            game.pushState(std::move(editorState));

            // Modifica minima: aggiungi ProcessEvent nel ciclo di eventi di Game
            game.run();

            ImGui::SFML::Shutdown();
            return 0;
        }



        std::cerr << "Argomento non valido: usa -server o -editor oppure lascia vuoto per il client.\n";
        return 1;
    }
    sf::RenderWindow window(sf::VideoMode(1280, 720), "World2AI");
    Game game;
    game.setWindow(&window); // IMPORTANTE: prima del resto
    game.init();             // ora possiamo chiamare updateView()

    game.pushState(std::make_unique<MenuState>(game));
    game.run();
    return 0;
}
