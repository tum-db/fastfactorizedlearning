#include <vector>
#include <unordered_set>
#include <string>
#include <iostream>
#include "variableOrder.h"
#include "regression.h"

ExtendedVariableOrder testCreate() {
  ExtendedVariableOrder root{"C"};
  ExtendedVariableOrder a{"A"};
  ExtendedVariableOrder e{"E"};
  ExtendedVariableOrder b{"B"};
  ExtendedVariableOrder d{"D"};
  a.addChild(b);
  e.addChild(d);
  root.addChild(a);
  root.addChild(e);

  return root;
}

ExtendedVariableOrder testCreate2() {
  ExtendedVariableOrder l{"Location"};
  ExtendedVariableOrder c{"Competitor", {l.getName()}};
  ExtendedVariableOrder p{"Product", {l.getName()}};
  ExtendedVariableOrder i{"Inventory", {l.getName(), p.getName()}};
  ExtendedVariableOrder s{"Sale", {p.getName()}};
  ExtendedVariableOrder b{"Branch", {l.getName(), p.getName(), i.getName()}};
  ExtendedVariableOrder cn{"Competition", {l.getName(), c.getName()}};
  ExtendedVariableOrder ss{"Sales", {p.getName(), s.getName()}};
  // c.addKey(l.getName());
  // p.addKey(l.getName());
  // i.addKey(l.getName());
  // i.addKey(p.getName());
  // s.addKey(p.getName());
  s.addChild(ss);
  i.addChild(b);
  c.addChild(cn);
  p.addChild(s);
  p.addChild(i);
  l.addChild(c);
  l.addChild(p);

  return l;
}

void recursiveTest(const ExtendedVariableOrder& root) {
  std::cerr << root.getName() << ' ' << root.isLeaf() << '\n';
  for (const ExtendedVariableOrder& x : root.getChildren()) {
    recursiveTest(x);
  }
}

int main() {
  // std::cout << std::boolalpha;
  // ExtendedVariableOrder testV{testCreate()};
  // recursiveTest(testV);
  // std::cerr << '\n';

  // std::cout << factorizeSQL(testV) << '\n';
  // std::cout << '\n';

  ExtendedVariableOrder testV2{testCreate2()};
  recursiveTest(testV2);
  std::cerr << '\n';

  std::cout << factorizeSQL(testV2) << '\n';
  std::cout << '\n';

  return 0;
}