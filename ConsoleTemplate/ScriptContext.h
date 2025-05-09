#pragma once
#include <string>
#include <sstream>
#include <unordered_set>

class ScriptContext {
public:
    ScriptContext();

    void startFunction(const std::string& name, const std::vector<std::string>& params = {});
    void endFunction();

    void declareVariable(const std::string& name, const std::string& initialValue = "nil");
    bool isVariableDeclared(const std::string& name) const;

    void emitLine(const std::string& code);
    std::string getScript() const;

    void pushIndent();
    void popIndent();
    void reset();

private:
    std::stringstream buffer;
    std::unordered_set<std::string> declaredVariables;
    int indentLevel;

    void writeIndent();
};
