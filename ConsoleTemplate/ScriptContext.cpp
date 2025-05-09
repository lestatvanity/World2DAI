#include "ScriptContext.h"

ScriptContext::ScriptContext() : indentLevel(0) {}

void ScriptContext::startFunction(const std::string& name, const std::vector<std::string>& params) {
    std::string paramList;
    for (size_t i = 0; i < params.size(); ++i) {
        paramList += params[i];
        if (i < params.size() - 1) paramList += ", ";
    }
    emitLine("function " + name + "(" + paramList + ")");
    pushIndent();
}

void ScriptContext::endFunction() {
    popIndent();
    emitLine("end");
}

void ScriptContext::declareVariable(const std::string& name, const std::string& initialValue) {
    if (!isVariableDeclared(name)) {
        writeIndent();
        buffer << name << " = " << initialValue << "\n";
        declaredVariables.insert(name);
    }
}

bool ScriptContext::isVariableDeclared(const std::string& name) const {
    return declaredVariables.count(name) > 0;
}

void ScriptContext::emitLine(const std::string& code) {
    writeIndent();
    buffer << code << "\n";
}

void ScriptContext::pushIndent() {
    ++indentLevel;
}

void ScriptContext::popIndent() {
    if (indentLevel > 0) --indentLevel;
}

void ScriptContext::reset() {
    buffer.str("");
    buffer.clear();
    declaredVariables.clear();
    indentLevel = 0;
}

std::string ScriptContext::getScript() const {
    return buffer.str();
}

void ScriptContext::writeIndent() {
    for (int i = 0; i < indentLevel; ++i)
        buffer << "    ";
}
