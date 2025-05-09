#pragma once
#include <string>

struct TileInfo {
    std::string name;           // Nome identificativo (es. "Fiore", "Pietra")
    std::string type;           // Tipo logico (es. "decor", "blocco", "porta")
    bool walkable = true;       // Camminabile sì/no
    std::string category;       // Categoria visiva o logica (es. "vegetazione", "acqua")
    std::string genre;          // Genere opzionale, usabile come ulteriore filtro
    std::string season = "all"; // per quali stagioni è visibile/usabile

    // Campi per animazione
    bool animated = false;
    int frameCount = 1;           // Quanti frame ha (default 1)
    float frameDuration = 0.1f;   // Durata in secondi per frame
};
