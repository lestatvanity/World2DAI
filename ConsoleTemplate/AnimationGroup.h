#pragma once
//#include "EditorState.h"
#include <string>
#include <vector>
#include <SFML/System/Vector2.hpp>
#include <nlohmann/json.hpp>

namespace sf {   // <── NB!
    inline void to_json(nlohmann::json& j, const Vector2i& v)
    {
        j = nlohmann::json{ v.x, v.y };
    }
    inline void from_json(const nlohmann::json& j, Vector2i& v)
    {
        j.at(0).get_to(v.x);
        j.at(1).get_to(v.y);
    }
}

//────────────────────────────
//  Strutture dati
//────────────────────────────
// Questo frame rappresenta un tile visualizzato in una posizione e offset specifici in un frame animato.
// Serve solo alla composizione visiva di un frame e non contiene dati logici o semantici.
struct AnimationGroupFrame {
    int tileID;                 // ID del tile nel tileset (corrisponde a un TileInfo già registrato)
    sf::Vector2i offset;        // Posizione relativa rispetto al punto di ancoraggio del frame

    // Operatore di confronto per backup e confronto automatico
    bool operator==(const AnimationGroupFrame& other) const {
        return tileID == other.tileID && offset == other.offset;
    }
};

// Un gruppo animato definisce una sequenza di frame che si ripetono nel tempo per formare un effetto visivo
// È una definizione condivisa che può essere piazzata in mappa come istanza
struct AnimationGroup {
    std::string name;                            // Nome univoco del gruppo animato
    float frameDuration = 0.20f;                 // Durata di ogni frame in secondi
    int cols = 1, rows = 1;                      // Dimensioni della griglia in cui i tile vengono composti
    std::vector<std::vector<AnimationGroupFrame>> frames; // Ogni frame è una lista di tile animati

    std::string tag = "";                        // Categoria o gruppo usato per filtro/tag
    std::string soundName = "";                  // Suono associato a questo gruppo (opzionale)
    float soundVolume = 100.f;                   // Volume del suono (da 0 a 100)
    bool soundLoop = true;                       // Il suono viene ripetuto in loop?

    // Operatore di confronto completo per salvataggio/backup
    bool operator==(const AnimationGroup& other) const {
        return name == other.name &&
            tag == other.tag &&
            frameDuration == other.frameDuration &&
            cols == other.cols &&
            rows == other.rows &&
            frames == other.frames &&
            soundName == other.soundName &&
            soundVolume == other.soundVolume &&
            soundLoop == other.soundLoop;
    }
};



//────────────────────────────
// Istanza piazzata sulla mappa
//────────────────────────────
struct AnimationGroupInstance {
    const AnimationGroup* def = nullptr; // puntatore alla definizione condivisa
    sf::Vector2i origin;                 // cella in alto-sinistra sul TileMap
    float time = 0.f;                    // timer per calcolare il frame attuale
    bool playing = false;

};

//────────────────────────────
//  Adattatori JSON
//────────────────────────────

// 1. Prima diciamo a nlohmann come gestire sf::Vector2i
inline void to_json(nlohmann::json& j, const sf::Vector2i& v) {
    j = nlohmann::json{ v.x, v.y };          // serializza come array [x, y]
}

inline void from_json(const nlohmann::json& j, sf::Vector2i& v) {
    j.at(0).get_to(v.x);
    j.at(1).get_to(v.y);
}

// 2. Ora le tue funzioni per AnimationGroupFrame
inline void to_json(nlohmann::json& j, const AnimationGroupFrame& f) {
    j["tileID"] = f.tileID;
    j["offset"] = f.offset;                  // usa gli adattatori di sf::Vector2i
}

inline void from_json(const nlohmann::json& j, AnimationGroupFrame& f) {
    j.at("tileID").get_to(f.tileID);
    j.at("offset").get_to(f.offset);
}

// 3. Funzioni per l’intero AnimationGroup
inline void to_json(nlohmann::json& j, const AnimationGroup& g) {
    j = nlohmann::json{
        {"name", g.name},
        {"frameDuration", g.frameDuration},
        {"cols", g.cols},
        {"rows", g.rows},
        {"frames", g.frames},
        {"soundName", g.soundName},
    {"soundVolume", g.soundVolume},
    {"soundLoop", g.soundLoop}
    };
}

inline void from_json(const nlohmann::json& j, AnimationGroup& g) {
    j.at("name").get_to(g.name);
    j.at("frameDuration").get_to(g.frameDuration);
    j.at("cols").get_to(g.cols);
    j.at("rows").get_to(g.rows);
    j.at("frames").get_to(g.frames);
    g.soundName = j.value("soundName", "");
    g.soundVolume = j.value("soundVolume", 100.f);
    g.soundLoop = j.value("soundLoop", true);
}
