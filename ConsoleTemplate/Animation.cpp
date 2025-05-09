#include "Animation.h"

// Costruttore con nome
Animation::Animation(const std::string& name)
    : name(name), currentFrameIndex(0), timer(0), loop(true) {}

// Aggiunge un frame alla lista
void Animation::addFrame(int x, int y, int durationMs) {
    frames.push_back({ x, y, durationMs }); // Inserisce un frame con coordinate e durata
}

// Restituisce il frame attuale
const AnimationFrame& Animation::getCurrentFrame() const {
    return frames[currentFrameIndex]; // Accesso diretto all'indice corrente
}

// Aggiorna l'animazione in base al tempo passato
void Animation::update(int deltaTime) {
    if (frames.empty()) return; // Evita operazioni se non ci sono frame

    timer += deltaTime; // Accumula il tempo
    if (timer >= frames[currentFrameIndex].durationMs) { // Se è tempo di cambiare frame
        timer = 0; // Reset timer
        if (currentFrameIndex + 1 < frames.size()) {
            currentFrameIndex++; // Passa al frame successivo
        }
        else if (loop) {
            currentFrameIndex = 0; // Riparte dal primo se in loop
        }
    }
}

// Resetta l'animazione al primo frame
void Animation::reset() {
    currentFrameIndex = 0; // Primo frame
    timer = 0;             // Reset del timer
}

// Restituisce il nome identificativo
const std::string& Animation::getName() const {
    return name; // Nome interno della classe
}

// Verifica se l'animazione ha finito (solo se non in loop)
bool Animation::isFinished() const {
    return !loop && currentFrameIndex == frames.size() - 1; // Fine solo se non in loop e ultimo frame
}

// Imposta la modalità loop
void Animation::setLoop(bool value) {
    loop = value; // Abilita/disabilita il loop
}

// Restituisce lo stato attuale del loop
bool Animation::getLoop() const {
    return loop; // True se animazione ciclica
}

// Imposta manualmente l'indice del frame corrente
void Animation::setFrameIndex(int index) {
    if (index >= 0 && index < static_cast<int>(frames.size())) {
        currentFrameIndex = index; // Imposta l'indice valido
        timer = 0;                 // Reset timer per sincronizzazione
    }
}

// Restituisce l'indice attuale del frame
int Animation::getFrameIndex() const {
    return currentFrameIndex; // Accesso all'indice corrente
}

// Salva su file i dati dell'animazione
void Animation::saveToFile(const std::string& path) const {
    std::ofstream out(path);
    if (!out.is_open()) return; // Verifica apertura file
    out << name << " " << loop << " " << frames.size() << "\n"; // Scrive intestazione
    for (const auto& f : frames)
        out << f.textureX << " " << f.textureY << " " << f.durationMs << "\n"; // Scrive ogni frame
}

// Carica i dati dell'animazione da file
bool Animation::loadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) return false; // Verifica apertura
    size_t frameCount;
    in >> name >> loop >> frameCount; // Legge intestazione
    frames.clear();
    for (size_t i = 0; i < frameCount; ++i) {
        AnimationFrame f;
        in >> f.textureX >> f.textureY >> f.durationMs; // Legge ogni frame
        frames.push_back(f);
    }
    reset(); // Ripristina animazione
    return true;
}