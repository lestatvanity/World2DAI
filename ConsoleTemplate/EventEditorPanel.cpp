// ──────────────────────────────────────────────────────────────
//  EventEditorPanel.cpp   –   prima implementazione funzionante
// ──────────────────────────────────────────────────────────────

#include "EventEditorPanel.h"
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include <filesystem>
#include <iostream>

using events::TargetType;
using events::EventData;


//---------------------------------------------------------------
// Helper locali
//---------------------------------------------------------------
static const char* targetTypeNames[] = { "Tile", "Group", "Entity" };

//---------------------------------------------------------------
void EventEditorPanel::render(bool* open)
{
    if (!open || !(*open)) return;

    if (!eventSystem)
    {
        ImGui::Begin("Editor Eventi", open);
        ImGui::Text("EventSystem non assegnato!");
        ImGui::End();
        return;
    }

    ImGui::Begin("Editor Eventi", open);

    //----------------------------------------------------
    // 1. Lista eventi esistenti
    //----------------------------------------------------
    if (ImGui::CollapsingHeader("Eventi esistenti", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::BeginTable("##events", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 30);
            ImGui::TableSetupColumn("Target", ImGuiTableColumnFlags_WidthFixed, 60);
            ImGui::TableSetupColumn("Detail", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Event", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Script", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 40);
            ImGui::TableHeadersRow();

            int idx = 0;
            for (auto& ev : eventSystem->getEvents())
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%d", idx);
                ImGui::TableSetColumnIndex(1); ImGui::Text("%s", targetTypeNames[(int)ev.targetType]);
                ImGui::TableSetColumnIndex(2);
                switch (ev.targetType)
                {
                case TargetType::Tile:   ImGui::Text("L%d (%d,%d)", ev.layer, ev.position.x, ev.position.y); break;
                case TargetType::Group:  ImGui::Text("%s @(%d,%d)", ev.groupName.c_str(), ev.position.x, ev.position.y); break;
                case TargetType::Entity: ImGui::Text("%s", ev.entityID.c_str()); break;
                }
                ImGui::TableSetColumnIndex(3); ImGui::Text("%s", ev.data.eventType.c_str());
                ImGui::TableSetColumnIndex(4); ImGui::Text("%s", ev.data.scriptPath.c_str());
                ImGui::TableSetColumnIndex(5);
                ImGui::PushID(idx);
                if (ImGui::SmallButton("X"))
                {
                    // rimuovi elemento
                    auto& vec = eventSystem->getEvents();
                    vec.erase(vec.begin() + idx);
                    ImGui::PopID();
                    ImGui::EndTable();
                    ImGui::End();
                    return; // evita iterare oltre indice invalido
                }
                ImGui::PopID();
                ++idx;
            }
            ImGui::EndTable();
        }
    }

    //----------------------------------------------------
    // 2. Form per aggiungere un nuovo evento
    //----------------------------------------------------
    if (ImGui::CollapsingHeader("Nuovo evento", ImGuiTreeNodeFlags_DefaultOpen))
    {
        static int tgtTypeIndex = 0;
        ImGui::Combo("Target##combo", &tgtTypeIndex, targetTypeNames, IM_ARRAYSIZE(targetTypeNames));

        static int layer = 0;
        static int posX = 0;
        static int posY = 0;
        static char groupBuf[64] = "";
        static char entityBuf[64] = "";
        static bool repeatable = true;

        // Input specifici per target
        TargetType ttype = (TargetType)tgtTypeIndex;
        if (ttype == TargetType::Tile)
        {
            ImGui::InputInt("Layer", &layer);
            ImGui::InputInt("X", &posX);
            ImGui::InputInt("Y", &posY);
        }
        else if (ttype == TargetType::Group)
        {
            ImGui::InputText("Nome gruppo", groupBuf, IM_ARRAYSIZE(groupBuf));
            ImGui::InputInt("Origin X", &posX);
            ImGui::InputInt("Origin Y", &posY);
        }
        else // Entity
        {
            ImGui::InputText("Entity ID", entityBuf, IM_ARRAYSIZE(entityBuf));
        }

        // Event type e script
        ImGui::InputText("Event type", eventTypeInput, IM_ARRAYSIZE(eventTypeInput));
        ImGui::InputText("Script path", scriptPathInput, IM_ARRAYSIZE(scriptPathInput));
        ImGui::Checkbox("Repeatable", &repeatable);

        if (ImGui::Button("Sfoglia script"))
        {
            IGFD::FileDialogConfig cfg;
            cfg.path = ".";          // directory di partenza
            ImGuiFileDialog::Instance()->OpenDialog("ChooseScript",
                "Seleziona script",
                "*.lua",
                cfg);
        }

        if (ImGuiFileDialog::Instance()->Display("ChooseScript"))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
                strncpy_s(scriptPathInput, path.c_str(), sizeof(scriptPathInput));
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGui::Button("Aggiungi evento"))
        {
            EventData data;
            data.eventType = eventTypeInput;
            data.scriptPath = scriptPathInput;
            data.repeatable = repeatable;

            switch (ttype)
            {
            case TargetType::Tile:
                eventSystem->addTileEvent(layer, { posX, posY }, data);
                break;
            case TargetType::Group:
                eventSystem->addGroupEvent(groupBuf, { posX, posY }, data);
                break;
            case TargetType::Entity:
                eventSystem->addEntityEvent(entityBuf, data);
                break;
            }
            // pulizia campi veloci
            scriptPathInput[0] = '\0';
        }
    }

    ImGui::End();
}
