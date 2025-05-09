// ──────────────────────────────────────────────────────────────
//  NodeGraph.cpp   –   primo skeleton funzionante (SFML + ImGui)
//  • Griglia pannabile/zoomabile
//  • Drag nodi con titolo e pin IN/OUT
//  • Creazione link trascinando da pin OUT a pin IN
//  • Salvataggio / caricamento JSON (nodo+link)      
//  Dipendenze:  SFML/Graphics  ImGui-SFML  nlohmann/json
// ----------------------------------------------------------------
#include "NodeGraph.h"
#include "AnimationBlueprintintUtils.h"
#include "EditorPlayerPanel.h"
#include <imgui.h>
#include <imgui-SFML.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <iostream>
//#include <queue>
#include <unordered_set>

// helper colore
static ImU32 rgba(int r, int g, int b, int a = 255) { return IM_COL32(r, g, b, a); }

// hitTestPin: verifica se mouse è dentro il cerchio del pin
static bool hitTestPin(const Node& node, const Pin& pin,
    const ImVec2& mouse, const ImVec2& origin,
    float zoom, const ImVec2& viewOffset)
{
    ImVec2 wp = { node.pos.x + pin.offset.x,
                  node.pos.y + pin.offset.y };
    ImVec2 sp = {
        origin.x + (wp.x - viewOffset.x) * zoom,
        origin.y + (wp.y - viewOffset.y) * zoom
    };
    float r = 5.f * zoom;
    float dx = mouse.x - sp.x;
    float dy = mouse.y - sp.y;
    return dx * dx + dy * dy <= r * r;
}

std::vector<CommentBox> commentBoxes;

//------------------------------------------------------------------
//  Ctor – aggiunge un nodo "Event" di prova
//------------------------------------------------------------------
NodeGraph::NodeGraph() : nextNodeId(0), selectedNodeId(-1), draggingLink(false), linkFromNode(-1), linkFromPin(-1),
dragNodeId(-1), selectingArea(false), errorBlinkTimer(0.0f), flowActive(false), flowTimer(0.0f), selectedCommentId(-1), dragCommentId(-1)
{
    
    Node n; 
    n.id = nextNodeId++;
    n.pos = { 50,50 }; 
    n.title = "onClick"; 
    n.type = "Event";
    n.pins.push_back({ 0,PinKind::Out,{n.size.x - 10, n.size.y / 2} });
    nodes.push_back(n);

    ScriptManagerInstance::set(&scriptManager); // rendo accessibile globalmente
    NodeLibrary::getNodes(); // forza inizializzazione una sola volta
  

}

//------------------------------------------------------------------
//  Coordinate helpers
//------------------------------------------------------------------
ImVec2 NodeGraph::worldToScreen(const ImVec2& p) const
{
    return ImVec2(
        (p.x - viewOffset.x) * zoom,
        (p.y - viewOffset.y) * zoom
    );
}
ImVec2 NodeGraph::screenToWorld(const ImVec2& p) const
{
    return ImVec2(
        p.x / zoom + viewOffset.x,
        p.y / zoom + viewOffset.y
    );
}

//------------------------------------------------------------------
//  
//------------------------------------------------------------------
//──────────────────────────────────────────────────────────────
//  draw() – NodeGraph disegna la griglia, nodi, link e pannello interattivo
//──────────────────────────────────────────────────────────────

void NodeGraph::draw()
{
    ImGui::Begin("Node Editor");

    // === Toolbar in alto ===
    if (ImGui::BeginChild("Toolbar", ImVec2(0, 40), false, ImGuiWindowFlags_NoScrollbar)) {
        if (ImGui::Button("Ricarica nodi dinamici")) {
            NodeLibrary::refresh();
        }
        ImGui::SameLine();
        if (ImGui::Button("Auto Layout")) autoLayout();
        ImGui::SameLine();
        if (ImGui::Button("Avvia Flow")) startFlow();
        // === Pulsante sviluppatore: genera Lua ===
        ImGui::SameLine();
        static char fileName[64] = "my_script.lua";
        ImGui::PushItemWidth(150);
        ImGui::InputText("##LuaFileName", fileName, IM_ARRAYSIZE(fileName));
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("Genera Lua")) {
            std::string path = "scripts/" + std::string(fileName);
            generateLua(path, scriptManager);
        }
    
    }
    ImGui::EndChild();

    // Inizio della finestra ImGui per la preview dell'animazione
    ImGui::Begin("Anteprima Animazione");

    // Ottiene il nome dell'animazione attualmente attiva (es. "idle")
    std::string currentAnim = previewComponent.getCurrentAnimationName();

    // Se è attiva un'animazione (quindi currentAnim NON è vuota)
    if (!currentAnim.empty()) {
        // Prende il frame corrente da disegnare
        const AnimationFrame& frame = previewComponent.getCurrentFrame();

        // Imposta il rettangolo visibile della texture (da sprite sheet)
        sf::IntRect rect(frame.textureX, frame.textureY, 96, 128);
        // Se conosci blueprint.frameWidth/frameHeight, puoi usare quelli

        // Applica il rettangolo alla sprite (indica quale parte della texture disegnare)
        previewSprite.setTextureRect(rect);

        // Calcola la posizione del mouse nella finestra ImGui
        ImVec2 pos = ImGui::GetCursorScreenPos();

        // Ottiene il puntatore alla finestra principale SFML (usato per disegnare la sprite)
        sf::RenderWindow* window = (sf::RenderWindow*)ImGui::GetMainViewport()->PlatformHandleRaw;

        // Se la finestra SFML è valida, disegna la sprite
        if (window) {
            previewSprite.setPosition(pos.x, pos.y); // posiziona lo sprite nella finestra ImGui
            window->draw(previewSprite);             // disegna lo sprite attuale del frame
        }

        // Riserva spazio nella finestra ImGui (altrimenti il layout salta)
        ImGui::Dummy(ImVec2(96, 128)); // deve corrispondere alla dimensione del frame

        // Mostra il nome dell'animazione attuale come testo nella GUI
        ImGui::Text("Animazione attuale: %s", currentAnim.c_str());
    }
    else {
        // Se nessuna animazione è attiva, mostra un messaggio
        ImGui::Text("Nessuna animazione attiva");
    }

    // Chiude il pannello "Anteprima Animazione"
    ImGui::End();





    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("NodeCanvas", canvasSize, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImVec2 mouse = ImGui::GetMousePos();

    // ----- draw grid -----
    float grid = 32.f * zoom;
    ImU32 gridCol = rgba(60, 60, 60, 60);
    for (float gx = fmodf(-viewOffset.x * zoom, grid); gx < canvasSize.x; gx += grid)
        dl->AddLine({ origin.x + gx, origin.y }, { origin.x + gx, origin.y + canvasSize.y }, gridCol);
    for (float gy = fmodf(-viewOffset.y * zoom, grid); gy < canvasSize.y; gy += grid)
        dl->AddLine({ origin.x, origin.y + gy }, { origin.x + canvasSize.x, origin.y + gy }, gridCol);

    // ----- draw links -----
    ImU32 linkCol = rgba(240, 200, 80, 200); // Definisci linkCol UNA SOLA VOLTA QUI

    for (const auto& L : links)
    {
        Node const& A = nodes[L.fromNode];
        Pin  const& PA = A.pins[L.fromPin];
        ImVec2 p0 = { origin.x + (A.pos.x + PA.offset.x - viewOffset.x) * zoom,
                      origin.y + (A.pos.y + PA.offset.y - viewOffset.y) * zoom };
        Node const& B = nodes[L.toNode];
        Pin  const& PB = B.pins[L.toPin];
        ImVec2 p3 = { origin.x + (B.pos.x + PB.offset.x - viewOffset.x) * zoom,
                      origin.y + (B.pos.y + PB.offset.y - viewOffset.y) * zoom };
        ImVec2 p1 = { p0.x + 50.f * zoom, p0.y };
        ImVec2 p2 = { p3.x - 50.f * zoom, p3.y };
        dl->AddBezierCubic(p0, p1, p2, p3, linkCol, 3.0f);
    }

    // link being dragged
    if (draggingLink && linkFromNode >= 0 && linkFromPin >= 0)
    {
        Node const& A = nodes[linkFromNode];
        Pin  const& PA = A.pins[linkFromPin];
        ImVec2 p0 = { origin.x + (A.pos.x + PA.offset.x - viewOffset.x) * zoom,
                      origin.y + (A.pos.y + PA.offset.y - viewOffset.y) * zoom };
        ImVec2 p3 = ImGui::GetMousePos();
        ImVec2 p1 = { p0.x + 50.f * zoom, p0.y };
        ImVec2 p2 = { p3.x - 50.f * zoom, p3.y };
        dl->AddBezierCubic(p0, p1, p2, p3, linkCol, 3.0f);
    }

    if (selectingArea)
    {
        ImVec2 a = worldToScreen(selectionStart);
        ImVec2 b = worldToScreen(selectionEnd);
        ImVec2 screenA = { origin.x + a.x, origin.y + a.y };
        ImVec2 screenB = { origin.x + b.x, origin.y + b.y };
        dl->AddRect(screenA, screenB, IM_COL32(0, 200, 255, 100), 2.0f);
    }

    // ----- draw comment boxes -----
    for (int i = 0; i < commentBoxes.size(); ++i) {
        CommentBox& cb = commentBoxes[i];

        ImVec2 screenP = ImVec2(
            origin.x + (cb.pos.x - viewOffset.x) * zoom,
            origin.y + (cb.pos.y - viewOffset.y) * zoom
        );
        ImVec2 screenSize = ImVec2(cb.size.x * zoom, cb.size.y * zoom);
        ImVec2 screenEnd = ImVec2(screenP.x + screenSize.x, screenP.y + screenSize.y);

        // Interazione
        ImGui::SetCursorScreenPos(screenP);
        ImGui::PushID(i);
        if (ImGui::InvisibleButton("##comment", screenSize)) {
            selectedCommentId = i;
        }

        // Movimento
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            cb.pos.x += delta.x / zoom;
            cb.pos.y += delta.y / zoom;
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
        }

        // Disegna commento
        dl->AddRectFilled(screenP, screenEnd, cb.color, 8.f);
        dl->AddText(ImVec2(screenP.x + 10, screenP.y + 10), IM_COL32_WHITE, cb.text.c_str());

        // Triangolino in basso a destra
        dl->AddTriangleFilled(
            ImVec2(screenEnd.x - 12, screenEnd.y),
            screenEnd,
            ImVec2(screenEnd.x, screenEnd.y - 12),
            IM_COL32(200, 200, 200, 160)
        );

        // Resize
        ImVec2 resizeHandleSize = ImVec2(16, 16);
        ImVec2 resizeHandlePos = ImVec2(screenEnd.x - resizeHandleSize.x, screenEnd.y - resizeHandleSize.y);
        ImGui::SetCursorScreenPos(resizeHandlePos);
        ImGui::InvisibleButton("##resize", resizeHandleSize);
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            cb.size.x += delta.x / zoom;
            cb.size.y += delta.y / zoom;
            cb.size.x = std::max(cb.size.x, 100.0f);
            cb.size.y = std::max(cb.size.y, 60.0f);
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
        }

        ImGui::PopID();
    }


    bool isCanvasHovered = ImGui::IsWindowHovered();
    
    bool isAnyNodeActive = false;
    bool isAnyCommentActive = false;

    // ----- draw nodes -----
    ImU32 normalBgCol = rgba(40, 50, 55, 220);
    ImU32 activeBgCol = rgba(60, 180, 255, 220);
    ImU32 normalHdCol = rgba(80, 90, 95, 255);
    ImU32 activeHdCol = rgba(80, 220, 255, 255);

    for (int i = 0; i < nodes.size(); ++i) {
        auto& N = nodes[i];

        ImVec2 sp = { origin.x + (N.pos.x - viewOffset.x) * zoom,
                      origin.y + (N.pos.y - viewOffset.y) * zoom };
        ImVec2 sz = { N.size.x * zoom, N.size.y * zoom };

        // Glow nodo selezionato
        if (selectedNodeId == N.id) {
            // Glow esterno soft in stile Blueprint
            ImVec2 center = { sp.x + sz.x / 2.0f, sp.y + sz.y / 2.0f };
            float baseRadius = std::max(sz.x, sz.y) * 0.65f;

            for (int g = 0; g < 3; ++g) {
                float radius = baseRadius + g * 5.0f;
                int alpha = 80 - g * 25;
                dl->AddCircleFilled(center, radius, IM_COL32(255, 255, 0, alpha));
            }

            // Bordo giallo selezione
            dl->AddRect(ImVec2(sp.x - 4, sp.y - 4), ImVec2(sp.x + sz.x + 4, sp.y + sz.y + 4), IM_COL32(255, 255, 0, 200), 6.0f, 0, 2.0f);

            //dl->AddRect(ImVec2(sp.x - 4, sp.y - 4), ImVec2(sp.x + sz.x + 4, sp.y + sz.y + 4), IM_COL32(255, 255, 0, 200), 6.0f, 0, 2.0f);
        }

        ImGui::SetCursorScreenPos(sp);
        ImVec2 btnSize = { sz.x - 20.f * zoom, sz.y };
        ImGui::InvisibleButton(("##node_" + std::to_string(N.id)).c_str(), btnSize);

        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            N.pos.x += delta.x / zoom;
            N.pos.y += delta.y / zoom;
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
            isAnyNodeActive = true;
        }

        if (N.hasError && ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Parametri mancanti:");
            for (const auto& key : N.missingParams)
                ImGui::BulletText("%s", key.c_str());
            ImGui::EndTooltip();
        }

        bool isActive = activeNodes.count(N.id) > 0;
        ImU32 baseColor = N.color;
        ImU32 bgCol = isActive ? IM_COL32(baseColor >> 24, (baseColor >> 16) & 0xFF, (baseColor >> 8) & 0xFF, 220)
            : IM_COL32(baseColor >> 24, (baseColor >> 16) & 0xFF, (baseColor >> 8) & 0xFF, 180);
        ImU32 hdCol = isActive ? activeHdCol : normalHdCol;

        dl->AddRectFilled(sp, { sp.x + sz.x, sp.y + sz.y }, bgCol, 6.0f);

        // Bordo lampeggiante se errore
        ImU32 borderColor;
        if (N.hasError) {
            float alpha = std::abs(std::sin(errorBlinkTimer * 6.28f));
            int a = static_cast<int>(100 + 155 * alpha);
            borderColor = IM_COL32(255, 60, 60, a);
        }
        else {
            borderColor = IM_COL32(255, 255, 255, 120);
        }

        dl->AddRect(sp, { sp.x + sz.x, sp.y + sz.y }, borderColor, 6.0f, 0, 2.5f);
        dl->AddRectFilled(sp, { sp.x + sz.x, sp.y + 20.f * zoom }, hdCol, 6.0f, ImDrawFlags_RoundCornersTop);
        dl->AddText({ sp.x + 6.f * zoom, sp.y + 3.f * zoom }, IM_COL32_WHITE, N.title.c_str());

        // ---- PINs ----
        for (auto const& pin : N.pins) {
            ImVec2 pp = {
                origin.x + (N.pos.x + pin.offset.x - viewOffset.x) * zoom,
                origin.y + (N.pos.y + pin.offset.y - viewOffset.y) * zoom
            };

            ImU32 pinColor = (pin.kind == PinKind::In) ? IM_COL32(70, 170, 255, 255)  // blu input
                : IM_COL32(255, 70, 70, 255); // rosso output
            float radius = 5.f * zoom;
            bool hovered = ImGui::IsMouseHoveringRect(ImVec2(pp.x - 7, pp.y - 7), ImVec2(pp.x + 7, pp.y + 7));

            if (hovered) {
                dl->AddCircleFilled(pp, radius + 3.f, IM_COL32(255, 255, 255, 100));
                ImGui::SetTooltip(pin.kind == PinKind::In ? "Pin Input" : "Pin Output");
            }

            dl->AddCircleFilled(pp, radius, pinColor);

            ImGui::SetCursorScreenPos({ pp.x - 5.f * zoom, pp.y - 5.f * zoom });
            ImGui::InvisibleButton(("##pin_" + std::to_string(N.id) + "_" + std::to_string(pin.id)).c_str(), { 10.f * zoom, 10.f * zoom });
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Pin %s", pin.kind == PinKind::In ? "Input" : "Output");


            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            
                handlePinClick(N, pin);


            }


        }

        // ---- BOTTONE X per eliminare ----
        ImVec2 closeBtnPos = { sp.x + sz.x - 20.f * zoom, sp.y + 2.f * zoom };
        ImGui::SetCursorScreenPos(closeBtnPos);
        ImGui::PushID(N.id);
        if (ImGui::Button("x")) {
            nodes.erase(nodes.begin() + i);
            if (selectedNodeId == N.id) selectedNodeId = -1;
            i--;
            ImGui::PopID();
            continue;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Elimina nodo");
        ImGui::PopID();
    }

    
    // Se clicchi nel vuoto del canvas mentre stai trascinando un link, annulla
    if (isCanvasHovered
        && ImGui::IsMouseReleased(ImGuiMouseButton_Left)
        && draggingLink)
    {
        // Controlla se il mouse è sopra **qualche pin**
        bool overPin = false;
        for (const auto& node : nodes) {
            for (const auto& pin : node.pins) {
                ImVec2 pp = {
                    origin.x + (node.pos.x + pin.offset.x - viewOffset.x) * zoom,
                    origin.y + (node.pos.y + pin.offset.y - viewOffset.y) * zoom
                };
                ImVec2 min = ImVec2(pp.x - 7.f, pp.y - 7.f);
                ImVec2 max = ImVec2(pp.x + 7.f, pp.y + 7.f);
                if (ImGui::IsMouseHoveringRect(min, max)) {
                    overPin = true;
                    break;
                }
            }
            if (overPin) break;
        }

        if (!overPin) {
            draggingLink = false;
            linkFromNode = -1;
            linkFromPin = -1;
        }
    }


    

    // link being dragged
    if (draggingLink && linkFromNode >= 0 && linkFromPin >= 0)
    {
        const Node& from = nodes[linkFromNode];
        const Pin& pOut = from.pins[linkFromPin];

        ImVec2 p0 = {
            origin.x + (from.pos.x + pOut.offset.x - viewOffset.x) * zoom,
            origin.y + (from.pos.y + pOut.offset.y - viewOffset.y) * zoom
        };
        ImVec2 p3 = mouse;
        ImVec2 p1 = { p0.x + 50.f * zoom, p0.y };
        ImVec2 p2 = { p3.x - 50.f * zoom, p3.y };

        // Trova se stai sopra un altro pin
        int hoveredNode = -1, hoveredPin = -1;
        for (const auto& N : nodes)
        {
            for (const auto& pin : N.pins)
            {
                ImVec2 pp = {
                    origin.x + (N.pos.x + pin.offset.x - viewOffset.x) * zoom,
                    origin.y + (N.pos.y + pin.offset.y - viewOffset.y) * zoom
                };
                float dist = std::hypot(mouse.x - pp.x, mouse.y - pp.y);
                if (dist < 6.f * zoom)
                {
                    hoveredNode = N.id;
                    hoveredPin = pin.id;
                    break;
                }
            }
        }

        // Se sei su un altro pin: verifica validità
        if (hoveredNode >= 0 && hoveredPin >= 0)
        {
            const Node& to = nodes[hoveredNode];
            const Pin& pIn = to.pins[hoveredPin];
            if (isLinkValid(from, pOut, to, pIn))
            {
                dl->AddBezierCubic(p0, p1, p2, p3, IM_COL32(255, 255, 0, 200), 3.0f);
                lastLinkWasInvalid = false;
            }
            else
            {
                dl->AddBezierCubic(p0, p1, p2, p3, IM_COL32(255, 60, 60, 255), 3.0f);
                lastLinkWasInvalid = true;
            }
        }
        else
        {
            dl->AddBezierCubic(p0, p1, p2, p3, IM_COL32(255, 255, 0, 150), 3.0f);
            lastLinkWasInvalid = false;
        }
    }


    // Context menu on right-click of the canvas (not over a node or comment)
    // Context menu con popup migliorato
    // Context menu con popup migliorato
    if (isCanvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !isAnyNodeActive && !isAnyCommentActive)
        ImGui::OpenPopup("NodeCreateMenu");

    if (ImGui::BeginPopup("NodeCreateMenu")) {
        static std::string selectedCategory = "";
        static char filter[64] = "";

        ImGui::InputText("Filtro", filter, IM_ARRAYSIZE(filter));

        ImGui::BeginChild("leftPane", ImVec2(150, 300), true);
        for (const auto& cat : NodeLibrary::getCategories()) {
            if (ImGui::Selectable(cat.c_str(), selectedCategory == cat)) {
                selectedCategory = cat;
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("rightPane", ImVec2(250, 300), true);
        if (!selectedCategory.empty()) {
            for (const auto& nodeDef : NodeLibrary::getNodesInCategory(selectedCategory)) {
                if (strlen(filter) == 0 || nodeDef.name.find(filter) != std::string::npos) {
                    if (ImGui::Selectable(nodeDef.name.c_str())) {
                        Node n;
                        n.id = nodes.size();
                        ImVec2 center = ImVec2(
                            viewOffset.x + (canvasSize.x * 0.5f) / zoom,
                            viewOffset.y + (canvasSize.y * 0.5f) / zoom
                        );

                        n.pos = center;
                        n.title = nodeDef.name;
                        n.type = nodeDef.type;
                        n.category = nodeDef.category;
                        n.color = nodeDef.color;
                        nodeDef.onCreate(n);
                        nodes.push_back(n);
                        triggerAutoGenerate();
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
        }

        if (ImGui::Selectable("Aggiungi Commento")) {
            CommentBox cb;
            cb.pos = canvasCenter;
            cb.size = { 200, 120 };
            cb.text = "Scrivi qui...";
            commentBoxes.push_back(cb);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndChild();

        ImGui::EndPopup();
    }

    // Interazione click su commento per editarlo
    for (int i = 0; i < commentBoxes.size(); ++i) {
        CommentBox& cb = commentBoxes[i];

        // Calcolo posizione schermo del commento
        ImVec2 screenP = ImVec2(
            origin.x + (cb.pos.x - viewOffset.x) * zoom,
            origin.y + (cb.pos.y - viewOffset.y) * zoom
        );

        ImVec2 screenSize = ImVec2(cb.size.x * zoom, cb.size.y * zoom);
        ImVec2 screenEnd = ImVec2(screenP.x + screenSize.x, screenP.y + screenSize.y);

        // Interazione
        ImGui::SetCursorScreenPos(screenP);
        ImGui::PushID(i);
        if (ImGui::InvisibleButton("comment", screenSize)) {
            selectedCommentId = i;
        }

        // Disegno del rettangolo e del testo
        dl->AddRectFilled(screenP, screenEnd, cb.color, 8.f);
        dl->AddText(ImVec2(screenP.x + 10, screenP.y + 10), IM_COL32_WHITE, cb.text.c_str());
        ImGui::PopID();
    }

    // Popup per modifica del commento selezionato
    if (selectedCommentId >= 0 && selectedCommentId < commentBoxes.size()) {
        CommentBox& cb = commentBoxes[selectedCommentId];

        // Calcolo posizione popup di modifica
        ImVec2 popupPos = ImVec2(
            origin.x + (cb.pos.x + 10.0f - viewOffset.x) * zoom,
            origin.y + (cb.pos.y + cb.size.y + 10.0f - viewOffset.y) * zoom
        );
        ImGui::SetNextWindowPos(popupPos, ImGuiCond_Appearing);

        ImGui::Begin("Commento", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        static char buffer[256];
        strncpy(buffer, cb.text.c_str(), sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0'; // sicurezza null terminator

        if (ImGui::InputTextMultiline("##text", buffer, IM_ARRAYSIZE(buffer))) {
            cb.text = buffer;
        }

        if (ImGui::Button("Chiudi")) {
            selectedCommentId = -1;
        }

        ImGui::End();
    }



    // === Finestra combinata ===
    if (ImGui::Begin("Script Data")) {
        if (ImGui::CollapsingHeader("Eventi disponibili da ScriptManager")) {
            for (const auto& ev : scriptManager.getAvailableEventsForType("entity")) {
                ImGui::BulletText("%s", ev.c_str());
                ImGui::SameLine();
                std::string buttonId = std::string("[+") + ev + "]";
                if (ImGui::SmallButton(buttonId.c_str())) {
                    Node n;
                    n.id = nodes.size();
                    ImVec2 center;
                    center.x = viewOffset.x + (canvasSize.x * 0.5f) / zoom;
                    center.y = viewOffset.y + (canvasSize.y * 0.5f) / zoom;

                    n.pos = center;
                    n.title = ev;
                    n.type = "Event";
                    n.category = "Event";
                    n.color = IM_COL32(80, 160, 255, 255);
                    n.pins.push_back({ 0, PinKind::Out, {n.size.x - 10, n.size.y / 2} });
                    nodes.push_back(n);
                }
            }
        }
        if (ImGui::CollapsingHeader("Variabili")) {
            for (const auto& [name, var] : scriptManager.getVariables()) {
                ImGui::BulletText("%s (%s) = %s", name.c_str(), var.type.c_str(), var.defaultValue.c_str());
            }
        }
        if (ImGui::CollapsingHeader("Funzioni")) {
            for (const auto& [name, func] : scriptManager.getFunctions()) {
                ImGui::BulletText("function %s(...)", name.c_str());
                ImGui::SameLine();
                std::string buttonId = std::string("[+F] ") + name;
                if (ImGui::SmallButton(buttonId.c_str())) {
                    Node n;
                    n.id = nodes.size();
                    ImVec2 center;
                    center.x = viewOffset.x + (canvasSize.x * 0.5f) / zoom;
                    center.y = viewOffset.y + (canvasSize.y * 0.5f) / zoom;

                    n.pos = center;
                    n.title = name;
                    n.type = "CallFunction";
                    n.category = "Function";
                    n.color = IM_COL32(180, 180, 180, 255);
                    n.parameters["funcName"] = name;
                    n.pins.push_back({ 0, PinKind::In, {10, n.size.y / 2} });
                    n.pins.push_back({ 1, PinKind::Out, {n.size.x - 10, n.size.y / 2} });
                    nodes.push_back(n);
                }
            }
        }
        if (ImGui::CollapsingHeader("Aggiungi Variabile")) {
            static char name[64] = "";
            static char defValue[64] = "";
            static int typeIndex = 0;
            const char* types[] = { "number", "string", "bool" };

            ImGui::InputText("Nome", name, IM_ARRAYSIZE(name));
            ImGui::Combo("Tipo", &typeIndex, types, IM_ARRAYSIZE(types));
            ImGui::InputText("Valore iniziale", defValue, IM_ARRAYSIZE(defValue));
            if (ImGui::Button("Aggiungi Variabile")) {
                ScriptVariable var;
                var.name = name;
                var.type = types[typeIndex];
                var.defaultValue = defValue;
                scriptManager.addVariable(var);
            }
        }
        if (ImGui::CollapsingHeader("Aggiungi Funzione")) {
            static char fname[64] = "";
            ImGui::InputText("Nome Funzione", fname, IM_ARRAYSIZE(fname));
            if (ImGui::IsItemDeactivatedAfterEdit() && strlen(fname) > 0) {
                ScriptFunction func;
                func.name = fname;
                scriptManager.addFunction(func);
            }
        }
    }
    ImGui::End();


    ImGui::Begin("Editor Player");

    ImGui::Text("Animazione assegnata al player:");
    static int selectedIndex = 0;

    // Costruisci la lista dei nomi degli stati animazione
    std::vector<std::string> animStates;
    for (const auto& node : nodes) {
        if (node.type == "AnimState") {
            animStates.push_back(node.parameters.at("stateName"));
        }
    }

    // Mostra dropdown solo se ci sono stati
    if (!animStates.empty()) {
        std::vector<const char*> items;
        for (const auto& s : animStates) items.push_back(s.c_str());

        if (ImGui::Combo("Animazione", &selectedIndex, items.data(), static_cast<int>(items.size()))) {
            selectedPlayerAnimState = animStates[selectedIndex];
            std::cout << "[EditorPlayer] Player userà animazione: " << selectedPlayerAnimState << "\n";

            // Applica alla preview
            if (animationManager.getAll().count(selectedPlayerAnimState)) {
                previewComponent.addAnimation(selectedPlayerAnimState, animationManager.get(selectedPlayerAnimState));
                previewComponent.play(selectedPlayerAnimState);
            }
        }
    }
    else {
        ImGui::Text("Nessun AnimState definito.");
    }

    ImGui::End();

 
    renderProperties();

    ImGui::EndChild();
    ImGui::End();
}



//------------------------------------------------------------------
//  handleEvent() – versione aggiornata con ALT+LMB per "calamita" nodo
//------------------------------------------------------------------
void NodeGraph::handleEvent(const sf::Event& e, const sf::RenderWindow&)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return; // ImGui sta usando il mouse, non fare altro
    }

    ImVec2 mouse = ImGui::GetMousePos();
    ImVec2 origin = ImGui::GetCursorScreenPos();

    static bool stickyDragging = false; // Modalità calamita attiva?

    

    // Se ALT + LMB su commento → attacca il commento al cursore
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) &&
        e.type == sf::Event::MouseButtonPressed &&
        e.mouseButton.button == sf::Mouse::Left)
    {
        ImVec2 mouse = ImGui::GetMousePos();
        ImVec2 origin = ImGui::GetCursorScreenPos();

        ImVec2 localMouse;
        localMouse.x = mouse.x - origin.x;
        localMouse.y = mouse.y - origin.y;

        for (int i = 0; i < commentBoxes.size(); ++i)
        {
            ImVec2 a = commentBoxes[i].pos;
            ImVec2 b;
            b.x = commentBoxes[i].pos.x + commentBoxes[i].size.x;
            b.y = commentBoxes[i].pos.y + commentBoxes[i].size.y;

            if (localMouse.x >= a.x && localMouse.x <= b.x &&
                localMouse.y >= a.y && localMouse.y <= b.y)
            {
                dragCommentId = i;

                dragCommentOffset.x = localMouse.x - commentBoxes[i].pos.x;
                dragCommentOffset.y = localMouse.y - commentBoxes[i].pos.y;

                break;
            }
        }
    }



    // MouseButtonReleased (LMB)
    if (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == sf::Mouse::Left)
    {
        
        if (stickyDragging)
        {
            stickyDragging = false; // Rilascia nodo dalla calamita
            dragNodeId = -1;
            return;
        }
        dragNodeId = -1; // Rilascia nodo normalmente
    }

    if (e.type == sf::Event::MouseMoved && dragNodeId != -1)
    {
        if (stickyDragging)
        {
            ImVec2 rel(mouse.x - origin.x, mouse.y - origin.y);
            nodes[dragNodeId].pos = screenToWorld(rel);
        }
        else
        {
            float worldX = (mouse.x - origin.x) / zoom + viewOffset.x;
            float worldY = (mouse.y - origin.y) / zoom + viewOffset.y;
            nodes[dragNodeId].pos.x = worldX - dragOffset.x;
            nodes[dragNodeId].pos.y = worldY - dragOffset.y;
        }

        // Sposta commento incollato
        if (e.type == sf::Event::MouseMoved && dragCommentId != -1)
        {
            ImVec2 mouse = ImGui::GetMousePos();
            ImVec2 origin = ImGui::GetCursorScreenPos();

            ImVec2 localMouse;
            localMouse.x = mouse.x - origin.x;
            localMouse.y = mouse.y - origin.y;

            commentBoxes[dragCommentId].pos.x = localMouse.x - dragCommentOffset.x;
            commentBoxes[dragCommentId].pos.y = localMouse.y - dragCommentOffset.y;
        }

    }

    if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left &&
        sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
    {
        selectingArea = true;
        ImVec2 rel(mouse.x - origin.x, mouse.y - origin.y);
        selectionStart = screenToWorld(rel);
        selectionEnd = selectionStart;
    }

    if (e.type == sf::Event::MouseMoved && selectingArea)
    {
        ImVec2 rel(mouse.x - origin.x, mouse.y - origin.y);
        selectionEnd = screenToWorld(rel);
    }

    if (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == sf::Mouse::Left && selectingArea)
    {
        selectingArea = false;
        selectedNodes.clear();

        ImVec2 minP(std::min(selectionStart.x, selectionEnd.x), std::min(selectionStart.y, selectionEnd.y));
        ImVec2 maxP(std::max(selectionStart.x, selectionEnd.x), std::max(selectionStart.y, selectionEnd.y));

        for (auto& node : nodes)
        {
            if (node.pos.x >= minP.x && node.pos.x <= maxP.x &&
                node.pos.y >= minP.y && node.pos.y <= maxP.y)
            {
                selectedNodes.insert(node.id);
            }
        }
    }

    if (sf::Mouse::isButtonPressed(sf::Mouse::Middle))
    {
        static ImVec2 last = mouse;
        ImVec2 delta = { (mouse.x - last.x) / zoom, (mouse.y - last.y) / zoom };
        viewOffset.x -= delta.x;
        viewOffset.y -= delta.y;
        last = mouse;
    }

    if (e.type == sf::Event::MouseWheelScrolled)
    {
        float factor = 1.f + e.mouseWheelScroll.delta * 0.1f;
        zoom = std::clamp(zoom * factor, 0.2f, 2.5f);
    }

    if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Delete)
    {
        if (selectedNodeId != -1)
        {
            links.erase(
                std::remove_if(links.begin(), links.end(), [&](const Link& l) {
                    return l.fromNode == selectedNodeId || l.toNode == selectedNodeId;
                    }),
                links.end()
            );

            nodes.erase(
                std::remove_if(nodes.begin(), nodes.end(), [&](const Node& n) {
                    return n.id == selectedNodeId;
                    }),
                nodes.end()
            );

            selectedNodeId = -1;
        }
    }
}



//------------------------------------------------------------------
void NodeGraph::update(float dt)
{
    errorBlinkTimer += dt;
    if (errorBlinkTimer > 1.0f) errorBlinkTimer = 0.0f;

    if (flowActive)
    {
        flowTimer += dt;
        if (flowTimer >= flowDelay && !flowQueque.empty() )
        {
            flowTimer = 0.0f;

            int current = flowQueque.front();
            flowQueque.pop();

            activeNodes.insert(current);

            for (const auto& link : links)
            {
                if (link.fromNode == current)
                {
                    flowQueque.push(link.toNode);
                }
            }
        }
        if (flowQueque.empty())
        {
            flowActive = false; // Finito!
        }
    }
}





void NodeGraph::save(const std::string& path) const
{
    nlohmann::json j;

    // Salva nodi
    for (const auto& n : nodes)
    {
        nlohmann::json nodeJ;
        nodeJ["id"] = n.id;
        nodeJ["pos"] = { n.pos.x, n.pos.y };
        nodeJ["size"] = { n.size.x, n.size.y };
        nodeJ["title"] = n.title;
        nodeJ["type"] = n.type;
        nodeJ["pins"] = nlohmann::json::array();

        for (const auto& p : n.pins)
        {
            nodeJ["pins"].push_back({
                { "id",    p.id },
                { "kind",  (p.kind == PinKind::In) ? "in" : "out" },
                { "offset", { p.offset.x, p.offset.y } }
                });
        }

        j["nodes"].push_back(nodeJ);
        nodeJ["parameters"] = n.parameters;
    }

    // Salva links
    for (const auto& l : links)
    {
        j["links"].push_back({
            { "fromNode", l.fromNode },
            { "fromPin",  l.fromPin  },
            { "toNode",   l.toNode   },
            { "toPin",    l.toPin    }
            });
    }

    std::ofstream out(path);
    if (out.is_open())
    {
        out << std::setw(4) << j;
        std::cout << "[NodeGraph] Salvato in " << path << "\n";
    }
    else
    {
        std::cerr << "[NodeGraph] ERRORE apertura file di salvataggio: " << path << "\n";
    }
}

void NodeGraph::load(const std::string& path)
{
    nodes.clear();
    links.clear();

    std::ifstream in(path);
    if (!in.is_open())
    {
        std::cerr << "[NodeGraph] ERRORE apertura file di caricamento: " << path << "\n";
        return;
    }

    nlohmann::json j;
    in >> j;

    for (const auto& nodeJ : j["nodes"])
    {
        Node n;
        n.id = nodeJ["id"];
        n.pos.x = nodeJ["pos"][0];
        n.pos.y = nodeJ["pos"][1];
        n.size.x = nodeJ["size"][0];
        n.size.y = nodeJ["size"][1];
        n.title = nodeJ["title"];
        n.type = nodeJ["type"];

        for (const auto& p : nodeJ["pins"])
        {
            Pin pin;
            pin.id = p["id"];
            pin.kind = (p["kind"] == "in") ? PinKind::In : PinKind::Out;
            pin.offset.x = p["offset"][0];
            pin.offset.y = p["offset"][1];
            n.pins.push_back(pin);
        }

        nodes.push_back(n);
        if (nodeJ.contains("parameters"))
            n.parameters = nodeJ["parameters"].get<std::unordered_map<std::string, std::string>>();

    }

    for (const auto& linkJ : j["links"])
    {
        Link l;
        l.fromNode = linkJ["fromNode"];
        l.fromPin = linkJ["fromPin"];
        l.toNode = linkJ["toNode"];
        l.toPin = linkJ["toPin"];
        links.push_back(l);
    }

    std::cout << "[NodeGraph] Caricato da " << path << "\n";
}

void NodeGraph::generateLua(const std::string& path, ScriptManager& scriptManager)
{
    for (auto& node : nodes) {
        if (node.type == "AnimState") {
            std::cout << "[DEBUG] Validating AnimState node...\n";
            std::vector<std::string> missing;
            if (!validateAnimStateNode(node, missing)) {
                node.hasError = true;
                node.missingParams = missing;
                std::cerr << "[AnimState] Parametri mancanti: ";
                for (const auto& p : missing) std::cerr << p << " ";
                std::cerr << "\n";
                continue;
            }
            else {
                node.hasError = false;
                node.missingParams.clear();
            }

        }
    }


    for (const auto& node : nodes) {
        if (node.type == "AnimState") {
            try {
                PlayerAnimationBlueprint blueprint = buildAnimationBlueprintFromNode(node);
                animationManager.loadFromBlueprint(blueprint);
                // Carica anche per anteprima se è "idle"
                if (blueprint.stateName == "idle") {
                    previewComponent.addAnimation("idle", animationManager.get("idle"));
                    previewComponent.play("idle");

                    if (previewTexture.loadFromFile(blueprint.texturePath)) {
                        previewSprite.setTexture(previewTexture);
                    }
                }
            }
            catch (...) {
                std::cerr << "[AnimState] Errore durante caricamento animazione. Nodo saltato.\n";
            }


            
        }
    }


    ScriptContext ctx;

    for (const auto& node : nodes)
    {
        if (node.type != "Event") continue;

        std::string eventName = node.title.empty() ? "onEvent" : node.title;
        ctx.startFunction(eventName, { "self" });

        std::function<void(int, int)> dfs = [&](int nodeId, int indent)
            {
                auto it = std::find_if(nodes.begin(), nodes.end(), [&](const Node& n) {
                    return n.id == nodeId;
                    });
                if (it == nodes.end()) return;

                const Node& current = *it;

                if (current.type == "SetVariable") {
                    std::string name = current.parameters.at("varName");
                    std::string val = current.parameters.at("value");
                    ctx.declareVariable(name, val);
                    ctx.emitLine(name + " = " + val);
                }
                else if (current.type == "CallFunction") {
                    std::string fname = current.parameters.at("funcName");

                    auto it = scriptManager.getFunctions().find(fname);
                    if (it == scriptManager.getFunctions().end()) {
                        ctx.emitLine("-- Errore: funzione '" + fname + "' non definita");
                        return;
                    }

                    const auto& func = it->second;
                    std::string call = fname + "(";

                    for (size_t i = 0; i < func.parameters.size(); ++i) {
                        const std::string& paramName = func.parameters[i];

                        if (current.parameters.count(paramName)) {
                            call += current.parameters.at(paramName);
                        }
                        else {
                            call += "nil";
                        }

                        if (i < func.parameters.size() - 1) call += ", ";
                    }

                    call += ")";
                    ctx.emitLine(call);
                }

                else if (current.type == "IfElse") {
                    std::string cond = current.parameters.at("condition");
                    ctx.emitLine("if " + cond + " then");
                    ctx.pushIndent();

                    int trueNode = -1, falseNode = -1;
                    for (const auto& l : links) {
                        if (l.fromNode == current.id) {
                            if (l.fromPin == 1) trueNode = l.toNode;
                            else if (l.fromPin == 2) falseNode = l.toNode;
                        }
                    }

                    if (trueNode != -1) dfs(trueNode, indent + 1);
                    ctx.popIndent();
                    ctx.emitLine("else");
                    ctx.pushIndent();
                    if (falseNode != -1) dfs(falseNode, indent + 1);
                    ctx.popIndent();
                    ctx.emitLine("end");
                }

                for (const auto& l : links) {
                    if (l.fromNode == current.id && current.type != "IfElse") {
                        dfs(l.toNode, indent);
                    }
                }
            };

        for (const auto& l : links) {
            if (l.fromNode == node.id)
                dfs(l.toNode, 1);
        }

        ctx.endFunction();
    }

    std::ofstream out(path);
    if (out.is_open()) {
        out << ctx.getScript();
        std::cout << "[NodeGraph] Script Lua generato in " << path << "\n";
    }
    else {
        std::cerr << "[NodeGraph] Errore apertura file: " << path << "\n";
    }
}



void NodeGraph::renderProperties()
{
    if (selectedNodeId == -1)
        return;

    // Trova il nodo selezionato
    auto it = std::find_if(nodes.begin(), nodes.end(), [&](const Node& n) {
        return n.id == selectedNodeId;
        });
    if (it == nodes.end())
        return;

    Node& node = *it;

    ImGui::Begin("Proprietà Nodo");

    ImGui::Text("ID: %d", node.id);
    ImGui::Text("Tipo: %s", node.type.c_str());

    if (node.type == "PlaySound")
    {
        static char buf[128];
        std::string soundName = node.parameters.count("sound") ? node.parameters["sound"] : "";
        strncpy(buf, soundName.c_str(), sizeof(buf));
        if (ImGui::InputText("Suono", buf, IM_ARRAYSIZE(buf)))
        {
            node.parameters["sound"] = std::string(buf);
        }
    }
    else if (node.type == "SetTile")
    {
        static char buf[32];
        std::string tileID = node.parameters.count("tileID") ? node.parameters["tileID"] : "";
        strncpy(buf, tileID.c_str(), sizeof(buf));
        if (ImGui::InputText("Tile ID", buf, IM_ARRAYSIZE(buf)))
        {
            node.parameters["tileID"] = std::string(buf);
        }
    }
    else if (node.type == "CallFunction") {
        // Lista di tutte le funzioni disponibili
        const auto& allFuncs = scriptManager.getFunctions();

        // Selezione del nome della funzione
        if (ImGui::BeginCombo("Funzione", node.parameters["funcName"].c_str())) {
            for (const auto& [name, func] : allFuncs) {
                bool isSelected = (node.parameters["funcName"] == name);
                if (ImGui::Selectable(name.c_str(), isSelected)) {
                    node.parameters["funcName"] = name;

                    // Quando selezioni una funzione, resetti i parametri previsti
                    for (const auto& param : func.parameters) {
                        node.parameters[param] = "";
                    }
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // Mostra i campi per i parametri
        std::string currentFunc = node.parameters["funcName"];
        auto it = allFuncs.find(currentFunc);
        if (it != allFuncs.end()) {
            for (const auto& param : it->second.parameters) {
                char buffer[256];
                std::string& value = node.parameters[param];
                std::strncpy(buffer, value.c_str(), sizeof(buffer));
                buffer[sizeof(buffer) - 1] = '\0';

                if (ImGui::InputText(("##" + param).c_str(), buffer, sizeof(buffer))) {
                    value = buffer;
                }

                ImGui::SameLine();
                ImGui::Text("%s", param.c_str());
            }
        }
    }

    else if (node.type == "AnimState")
    {
        char buf[128];

        strcpy(buf, node.parameters["stateName"].c_str());
        if (ImGui::InputText("State Name", buf, IM_ARRAYSIZE(buf)))
            node.parameters["stateName"] = buf;

        strcpy(buf, node.parameters["texturePath"].c_str());
        if (ImGui::InputText("Texture Path", buf, IM_ARRAYSIZE(buf)))
            node.parameters["texturePath"] = buf;

        strcpy(buf, node.parameters["frameWidth"].c_str());
        if (ImGui::InputText("Frame Width", buf, IM_ARRAYSIZE(buf)))
            node.parameters["frameWidth"] = buf;

        strcpy(buf, node.parameters["frameHeight"].c_str());
        if (ImGui::InputText("Frame Height", buf, IM_ARRAYSIZE(buf)))
            node.parameters["frameHeight"] = buf;

        strcpy(buf, node.parameters["rows"].c_str());
        if (ImGui::InputText("Rows", buf, IM_ARRAYSIZE(buf)))
            node.parameters["rows"] = buf;

        strcpy(buf, node.parameters["cols"].c_str());
        if (ImGui::InputText("Columns", buf, IM_ARRAYSIZE(buf)))
            node.parameters["cols"] = buf;

        strcpy(buf, node.parameters["frameTime"].c_str());
        if (ImGui::InputText("Frame Time", buf, IM_ARRAYSIZE(buf)))
            node.parameters["frameTime"] = buf;

        bool loop = node.parameters["loop"] == "true";
        if (ImGui::Checkbox("Loop", &loop))
            node.parameters["loop"] = loop ? "true" : "false";

        bool dir = node.parameters["directional"] == "true";
        if (ImGui::Checkbox("Directional", &dir))
            node.parameters["directional"] = dir ? "true" : "false";
    }


    else
    {
        ImGui::Text("Questo nodo non ha parametri editabili.");
    }

    ImGui::End();
}

std::optional<std::pair<int, int>> NodeGraph::pinAtDetailed(const ImVec2& mouse)
{
    ImVec2 origin = ImGui::GetCursorScreenPos();

    for (const auto& node : nodes)
    {
        for (const auto& pin : node.pins)
        {
            ImVec2 wp = { node.pos.x + pin.offset.x, node.pos.y + pin.offset.y };
            ImVec2 sp = {
                origin.x + (wp.x - viewOffset.x) * zoom,
                origin.y + (wp.y - viewOffset.y) * zoom
            };
            float r = 5.0f * zoom;
            float dx = mouse.x - sp.x;
            float dy = mouse.y - sp.y;
            if (dx * dx + dy * dy <= r * r)
                return std::make_pair(node.id, pin.id);
        }
    }
    return std::nullopt;
}


int NodeGraph::pinAt(const ImVec2& mouse)
{
    ImVec2 origin = ImGui::GetCursorScreenPos();

    for (const auto& node : nodes)
    {
        for (const auto& pin : node.pins)
        {
            ImVec2 wp = { node.pos.x + pin.offset.x, node.pos.y + pin.offset.y };
            ImVec2 sp = {
                origin.x + (wp.x - viewOffset.x) * zoom,
                origin.y + (wp.y - viewOffset.y) * zoom
            };
            float r = 5.0f * zoom;

            float dx = mouse.x - sp.x;
            float dy = mouse.y - sp.y;
            if (dx * dx + dy * dy <= r * r)
                return pin.id; // trovato il pin
        }
    }
    return -1; // nessun pin sotto il mouse
}

void NodeGraph::autoLayout()
{
    // Spaziatura
    const float nodeWidth = 200.0f;
    const float nodeHeight = 100.0f;
    const float hSpacing = 250.0f;
    const float vSpacing = 150.0f;

    // Trova tutti i nodi di partenza (Event)
    std::vector<int> startingNodes;
    for (const auto& n : nodes)
    {
        if (n.type == "Event")
            startingNodes.push_back(n.id);
    }

    ImVec2 startPos(50, 50);
    int level = 0;

    for (int eventId : startingNodes)
    {
        std::queue<std::pair<int, int>> q;
        std::unordered_set<int> visited;
        q.push({ eventId, 0 });
        visited.insert(eventId);

        std::unordered_map<int, int> levels;

        while (!q.empty())
        {
            auto [nodeId, depth] = q.front();
            q.pop();

            levels[nodeId] = depth;

            for (const auto& lnk : links)
            {
                if (lnk.fromNode == nodeId && visited.find(lnk.toNode) == visited.end())
                {
                    q.push({ lnk.toNode, depth + 1 });
                    visited.insert(lnk.toNode);
                }
            }
        }

        // Applica posizioni
        std::unordered_map<int, int> verticalOffset; // vertical position per ogni depth
        for (auto& n : nodes)
        {
            if (levels.find(n.id) != levels.end())
            {
                int d = levels[n.id];
                int offset = verticalOffset[d]++;
                n.pos.x = startPos.x + d * hSpacing;
                n.pos.y = startPos.y + offset * vSpacing;
            }
        }
        startPos.y += 600; // separa i vari grafi eventi
    }
}

void NodeGraph::startFlow()
{
    activeNodes.clear();
    while (!flowQueque.empty()) flowQueque.pop();

    for (const auto& node : nodes)
    {
        if (node.type == "Event")
        {
            flowQueque.push(node.id);
        }
    }

    if (!flowQueque.empty())
    {
        flowActive = true;
        flowTimer = 0.0f;
    }
}


bool NodeGraph::isLinkValid(const Node& fromNode, const Pin& fromPin, const Node& toNode, const Pin& toPin) const
{
    if (fromPin.kind != PinKind::Out || toPin.kind != PinKind::In)
        return false; // puoi collegare solo da OUT a IN

    if (fromNode.id == toNode.id)
        return false; // non puoi collegare a te stesso

    if (fromNode.type == "Event" && toNode.type == "Event")
        return false; // evento → evento vietato

    if (fromNode.type == "Event" && toPin.kind != PinKind::In)
        return false;

    // Evita doppi link sullo stesso pin
    for (const auto& l : links)
    {
        if (l.fromNode == fromNode.id && l.fromPin == fromPin.id &&
            l.toNode == toNode.id && l.toPin == toPin.id)
            return false;
    }

    return true;
}


void NodeGraph::handlePinClick(const Node& N, const Pin& pin)
{
    //std::cout << "[DEBUG] Click su pin tipo: " << (pin.kind == PinKind::In ? "IN" : "OUT") << " N.id = " << N.id << " pin.id = " << pin.id << "\n";

    // Click su stesso pin OUT → annulla dragging
    if (draggingLink && pin.kind == PinKind::Out && N.id == linkFromNode && pin.id == linkFromPin) {
        draggingLink = false;
        linkFromNode = -1;
        linkFromPin = -1;
        std::cout << "[DEBUG] Dragging annullato (secondo click su stesso pin OUT)\n";
        return;
    }

    //std::cout << "[DEBUG] Click su pin tipo: " << (pin.kind == PinKind::In ? "IN" : "OUT") << " N.id = " << N.id << " pin.id = " << pin.id << "\n";

    // Click su pin IN mentre si sta trascinando un link
    if (draggingLink && pin.kind == PinKind::In) {
        std::cout << "[DEBUG] Click su pin tipo: " << (pin.kind == PinKind::In ? "IN" : "OUT") << " N.id = " << N.id << " pin.id = " << pin.id << "\n";

        const Node& from = nodes[linkFromNode];
        const Pin& pOut = from.pins[linkFromPin];
        const Node& to = N;
        const Pin& pIn = pin;

        std::cout << "[DEBUG] Tentativo connessione verso pin IN: node " << to.id << " pin " << pIn.id << "\n";
        if (isLinkValid(from, pOut, to, pIn)) {
            links.push_back({ from.id, pOut.id, to.id, pIn.id });
            std::cout << "[DEBUG] Collegamento valido! Link creato da " << from.id << " a " << to.id << "\n";
        }
        else {
            std::cout << "[DEBUG] Collegamento non valido\n";
        }

        draggingLink = false;
        linkFromNode = -1;
        linkFromPin = -1;
        return;
    }

    // Inizio dragging da pin OUT
    if (!draggingLink && pin.kind == PinKind::Out) {
        draggingLink = true;
        linkFromNode = N.id;
        linkFromPin = pin.id;
        std::cout << "[DEBUG] Inizio dragging dal pin OUT: node " << N.id << " pin " << pin.id << "\n";
        return;
    }
}
