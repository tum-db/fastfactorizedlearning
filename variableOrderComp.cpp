#include <vector>
// #include <unordered_set>
#include <string>
#include <functional>
#include "variableOrderComp.h"

ExtendedVariableOrder::~ExtendedVariableOrder() {}
ExtendedVariableOrder::ExtendedVariableOrder(variable name, std::vector<variable> key)
    : m_name{name}, m_key{key} {}

const variable& ExtendedVariableOrder::getName() const { return m_name; }

const std::vector<variable>& ExtendedVariableOrder::getKey() const { return m_key; }

bool ExtendedVariableOrderLeaf::isLeaf() const { return true; }

ExtendedVariableOrderLeaf::ExtendedVariableOrderLeaf(variable name, std::vector<variable> key)
    : ExtendedVariableOrder{name, key} {}

bool ExtendedVariableOrderNode::isLeaf() const { return false; }

const std::vector<ExtendedVariableOrder*>& ExtendedVariableOrderNode::getChildren() const {
  return m_children;
}

void ExtendedVariableOrderNode::addChild(ExtendedVariableOrder* child) { m_children.push_back(child); }

ExtendedVariableOrderNode::~ExtendedVariableOrderNode() {
  for (auto x : m_children) {
    delete x;
  }
}
ExtendedVariableOrderNode::ExtendedVariableOrderNode(variable name, std::vector<variable> key)
    : ExtendedVariableOrder{name, key}, m_children{} {}