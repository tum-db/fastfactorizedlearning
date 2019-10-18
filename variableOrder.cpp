#include <vector>
// #include <unordered_set>
#include <string>
// #include <functional>
#include "variableOrder.h"

bool ExtendedVariableOrder::isLeaf() const { return m_children.empty(); }

const variable& ExtendedVariableOrder::getName() const { return m_name; }

const std::vector<variable>& ExtendedVariableOrder::getKey() const { return m_key; }

const std::vector<ExtendedVariableOrder>& ExtendedVariableOrder::getChildren() const { return m_children; }

void ExtendedVariableOrder::addChild(const ExtendedVariableOrder& child) { m_children.push_back(child); }

// void ExtendedVariableOrder::addKey(const variable& var) { m_key.insert(var); }

ExtendedVariableOrder::~ExtendedVariableOrder() {}
ExtendedVariableOrder::ExtendedVariableOrder(variable name, std::vector<variable> key)
    : m_name{name}, m_key{key}, m_children{} {}