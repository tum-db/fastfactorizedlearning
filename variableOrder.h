#ifndef VARIABLEORDER_H
#define VARIABLEORDER_H

#include <pqxx/pqxx>
#include <vector>
#include <string>

typedef std::string variable;

typedef struct {
  double avg;
  double max;
} scaleFactors;

class ExtendedVariableOrder {
 private:
  variable m_name;
  std::vector<variable> m_key;
  std::vector<ExtendedVariableOrder> m_children;
  const bool m_categorical;

 public:
  const variable& getName() const;
  const std::vector<variable>& getKey() const;
  const std::vector<ExtendedVariableOrder>& getChildren() const;
  
  void findLeaves(std::vector<ExtendedVariableOrder*>& leaves);

  bool isLeaf() const;
  bool isCategorical() const;

  void addChild(const ExtendedVariableOrder& child);
  // void addKey(const variable& var)

  // restrict access of convertName without allowing access to all members
  class nameKey {
   private:
    nameKey() {}
    friend std::vector<scaleFactors> scaleFeatures(const std::vector<std::string>& relevantColumns,
                                                   std::vector<ExtendedVariableOrder*>& leaves,
                                                   pqxx::connection& c, bool useRange);
  };

  void convertName(nameKey);

  ~ExtendedVariableOrder();
  ExtendedVariableOrder(const variable& name, const std::vector<variable>& key = {},
                        const bool categorical = false);
};

#endif