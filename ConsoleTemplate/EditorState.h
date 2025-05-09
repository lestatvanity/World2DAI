#pragma once
#include "GameState.h"
#include "TileMap.h"
#include "Entity.h"
//#include "TileSelectorPanel.h"
#include "TilesetEditorPanel.h"
#include "TileSelectorPanel.h"
#include "AnimationGroupEditorPanel.h"   // nuovo include
#include "TilesetManager.h"
#include "PropertyPanel.h"
#include "AnimationPanel.h"
#include "AnimationManager.h"
#include "AnimationComponent.h"
#include "ScriptManager.h"
#include "EntityPropertyPanel.h"
#include "WorldSettingsManager.h"
#include "WorldConfig.h"
#include "SoundManager.h"
#include "EventSystem.h"
#include "EventEditorPanel.h"
#include "NodeGraph.h"
#include "EditorPlayerPanel.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <fstream>
#include <optional>


//extern bool showTilesetEditor;


//WorldSettingsManager* worldSettings = nullptr;
//bool showWorldSettings = false;

enum class DrawTool {
    Brush,
    Fill,
    Rectangle,
    Selector,
    Event
};

struct PlacedAnimationGroup {
    std::string groupName;
    sf::Vector2i position;
    int layer;
};





class EditorState : public GameState {
public:
    EditorState(Game& game); // nessun path
    void initFromConfig(const std::string& configPath); // <-- nuova funzione

    void handleInput(sf::Event& event) override;
    void update(float dt) override;
    void render() override;
    void drawTransparentBackground(ImDrawList* drawList, ImVec2 pos, float size, float squareSize = 4.f);
    void updateBrushing();
    void generaMappaAutomatica(const std::string& stagione);
    void renderWorldConfigPanel();
    sf::Color getBackgroundColor() const;
    void renderTilesetDropdown();
    std::unique_ptr<WorldSettingsManager> worldSettings;
    bool showWorldSettings = false;
    void loadPlacedGroups(const std::string& path);
    void savePlacedGroups(const std::string& path);
    void loadFullMap(const std::string& mapName);
    void saveFullMap(const std::string& mapName);
    void autoLoadTilesetsIfMissing();
    void deleteTileAndGroupAt(int x, int y);
    void deleteTilesAndGroupsInSelection();
    void pushUndoState();
    void undo();
    void redo();
    void validateAnimationGroups();
    void handleEventSelection(int x, int y);
    void updateHoverSelection();
    sf::RectangleShape getHighlightRectangle(const SelectedEntity& entity);
    //static bool showGroupEditor = false;


private:
    std::unique_ptr<SoundManager> soundManager;  //  gestito da EditorState
    std::unique_ptr<EventEditorPanel> eventEditorPanel;   // <── nuovo

    std::vector<PlacedAnimationGroup> placedGroups;
    bool placingGroup = false;
    // EditorState.h  (sezione membri privati)
    bool showEventEditor = false;

    std::string currentGroupToPlace;
    // Editor GUI panels
    //TileSelectorPanel tilePanel;
    PropertyPanel propertyPanel;
    AnimationPanel animationPanel;
    //worldSettings = new WorldSettingsManager(config, game);
    WorldConfig config; // questo deve esistere!
    // --- Zoom & view ---
    sf::View gameView;
    sf::Vector2f originalViewSize;
    float currentZoom = 1.0f;
    TileSelectorPanel tileSelectorPanel;
    // === TILE CONFIG TOOL ===
    int currentLayerIndex = BASE_LAYER; // default
    int tileConfigID = 0;
    char tileName[64] = "Nuovo Tile";
    char tileType[64] = "terra";
    bool tileWalkable = true;
    char tileCategory[64] = "ambiente";
    char tileGenre[64] = "generico";
    bool showWorldConfigPanel = false;
    bool showTilesetEditor = false; // <-- ok così se è membro
    std::unique_ptr<AnimationGroupEditorPanel> groupEditor;
    bool showGroupEditor = false;
    // Tilemap + data
    std::unique_ptr<TileMap> tilemap;
    std::unique_ptr<TilesetEditorPanel> tilesetEditor;
    sf::Texture tilemapTexture;
    sf::Texture tileSelectorTexture;

    std::unordered_map<int, TileInfo> tileMetadata;
    //std::unique_ptr<TilesetEditorPanel> tilesetEditor;
    // Selezione tile
    int currentTileID = 0;
    int maxTileID = 0;
    int tileSize;
    int mapWidth;
    int mapHeight;
    std::string configPath;
    int tileScreenSize = 48; // visivamente sarà 48x48 sullo schermo
    sf::Vector2i selectionStart = { -1, -1 };
    sf::Vector2i selectionEnd = { -1, -1 };
    bool isSelectingArea = false;
    bool selectionActive = false;
    float selectionFadeTimer = 0.0f; // Timer che fa "respirare" la selezione
    events::EventSystem eventSystem;




    // Tile selector
    sf::VertexArray tileSelectorGrid;
    sf::RectangleShape selectedTileBox;
    sf::RectangleShape selectedTileDisplay;
    sf::Sprite tilePreview;

    // GUI font & testo
    sf::Font font;
    sf::Text tileIDText;
    sf::Text tileDescText;

    // Editor tools
    std::vector<std::string> tilesetPaths;
    int currentTilesetIndex = 0;
    bool showGrid = false;

    // Entità selezionata
    Entity* entitySelezionata = nullptr;

    // Motori
    ScriptManager scriptManager;
    AnimationManager animationManager;
    //TileSelectorPanel tilePanel;


    DrawTool currentTool = DrawTool::Brush;
    sf::Vector2i rectangleStart = { -1, -1 }; // per rettangoli

    // Metodi interni
    void buildTileSelectorGrid();
    void saveMap(const std::string& path);
    void loadMap(const std::string& path);
    void salvaTileMetadata(const std::string& path);
    void updateView();
    void toggleFullscreen();
    void loadTileMetadata(const std::string& path);
    void loadAvailableTilesets(const std::string& rootFolder);
    void loadTilesetFromFolder(const std::string& folderPath);
    void handleTilePlacement(sf::Event& event);
    void renderTilesetView();
    void handleEdgeScrolling(float dt);
    std::unique_ptr<TileSetManager> tileSetManager;
    std::string currentTilesetName = "terra"; // o il nome di default
    TileSetManager tilesetManager;
    std::vector<std::vector<std::vector<std::vector<TileInstance>>>> undoStack;
    std::vector<std::vector<std::vector<std::vector<TileInstance>>>> redoStack;
    SelectedEntity hoveredEntity;
    SelectedEntity selectedEntity;
    NodeGraph nodeGraph;
    bool openNodeGraph = false;

    std::optional<EditorPlayerPanel> playerPanel;
    AnimationComponent previewComponent;
    bool showPlayerPanel = false;

};
