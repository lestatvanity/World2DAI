#pragma once
#include "EventSystem.h"     // basta questo, PropertyPanel non serve

class EventEditorPanel {
public:
    void render(bool* open);                                 // <-- tolto il vector<EventData>
    void setEventSystem(events::EventSystem* es) { eventSystem = es; }
private:
    char eventTypeInput[64] = "onClick";
    char scriptPathInput[128] = "";
    events::EventSystem* eventSystem = nullptr;              // qualificato
};
