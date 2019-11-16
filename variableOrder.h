#ifndef VARIABLEORDER_H
#define VARIABLEORDER_H

#include <vector>
// #include <unordered_set>
#include <string>
// #include <functional>

typedef std::string variable;

class ExtendedVariableOrder {
 private:
  const variable m_name;
  std::vector<variable> m_key;
  std::vector<ExtendedVariableOrder> m_children;
  const bool m_categorical;

 public:
  const variable& getName() const;
  const std::vector<variable>& getKey() const;
  const std::vector<ExtendedVariableOrder>& getChildren() const;

  bool isLeaf() const;
  bool isCategorical() const;

  void addChild(const ExtendedVariableOrder& child);
  // void addKey(const variable& var);

  ~ExtendedVariableOrder();
  ExtendedVariableOrder(const variable& name, const std::vector<variable>& key = {}, const bool categorical = false);
};

#endif