#include <vector>
#include <string>
#include <cassert>
#include "variableOrder.h"

bool ExtendedVariableOrder::isLeaf() const { return m_children.empty(); }

bool ExtendedVariableOrder::isCategorical() const { return m_categorical; }

const variable& ExtendedVariableOrder::getName() const { return m_name; }

const std::vector<variable>& ExtendedVariableOrder::getKey() const { return m_key; }

const std::vector<ExtendedVariableOrder>& ExtendedVariableOrder::getChildren() const { return m_children; }

void ExtendedVariableOrder::findLeaves(std::vector<ExtendedVariableOrder*>& leaves) {
  if (isLeaf()) {
    leaves.push_back(this);
    return;
  }

  for (auto& x : m_children) {
    x.findLeaves(leaves);
  }
}

void ExtendedVariableOrder::addChild(const ExtendedVariableOrder& child) { m_children.push_back(child); }

// void ExtendedVariableOrder::addKey(const variable& var) { m_key.insert(var); }

void ExtendedVariableOrder::convertName(nameKey) { m_name += "_conv"; }

ExtendedVariableOrder::~ExtendedVariableOrder() {}
ExtendedVariableOrder::ExtendedVariableOrder(const variable& name, const std::vector<variable>& key,
                                             const bool categorical)
    : m_name{name}, m_key{key}, m_children{}, m_categorical{categorical} {
  assert(name.size() > 0);
}