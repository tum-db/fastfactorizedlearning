#ifndef VARIABLEORDER_H
#define VARIABLEORDER_H

#include <vector>
// #include <unordered_set>
#include <string>
// #include <functional>

typedef std::string variable;

class ExtendedVariableOrder {
 protected:
  variable m_name;
  std::vector<variable> m_key;

 public:
  const variable& getName() const;
  const std::vector<variable>& getKey() const;
  virtual bool isLeaf() const = 0;

  virtual ~ExtendedVariableOrder();
  ExtendedVariableOrder(variable name, std::vector<variable> key = {});
};

class ExtendedVariableOrderLeaf : public ExtendedVariableOrder {
 public:
  virtual bool isLeaf() const override;

  ExtendedVariableOrderLeaf(variable name, std::vector<variable> key = {});
};

class ExtendedVariableOrderNode : public ExtendedVariableOrder {
 private:
  std::vector<ExtendedVariableOrder*> m_children;

 public:
  virtual bool isLeaf() const override;
  const std::vector<ExtendedVariableOrder*>& getChildren() const;

  void addChild(ExtendedVariableOrder* child);

  ~ExtendedVariableOrderNode();
  ExtendedVariableOrderNode(variable name, std::vector<variable> key = {});
};

#endif