#ifndef VARIABLEORDER_H
#define VARIABLEORDER_H

#include <vector>
// #include <unordered_set>
#include <string>
// #include <functional>

typedef std::string variable;

class ExtendedVariableOrder {
 protected:
  const variable m_name;
  std::vector<variable> m_key;
  std::vector<ExtendedVariableOrder> m_children;

 public:
  const variable& getName() const;
  const std::vector<variable>& getKey() const;
  const std::vector<ExtendedVariableOrder>& getChildren() const;

  bool isLeaf() const;

  void addChild(const ExtendedVariableOrder& child);
  // void addKey(const variable& var);

  ~ExtendedVariableOrder();
  ExtendedVariableOrder(variable name, std::vector<variable> key = {});
};

#endif