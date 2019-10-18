#include <vector>
#include <unordered_set>
#include <string>
#include <iostream>
#include "variableOrderComp.h"
#include "regression.h"

ExtendedVariableOrder* testCreate() {
  ExtendedVariableOrderNode* root{new ExtendedVariableOrderNode{"C"}};
  ExtendedVariableOrderNode* a{new ExtendedVariableOrderNode{"A"}};
  ExtendedVariableOrderNode* e{new ExtendedVariableOrderNode{"E"}};
  ExtendedVariableOrderLeaf* b{new ExtendedVariableOrderLeaf{"B"}};
  ExtendedVariableOrderLeaf* d{new ExtendedVariableOrderLeaf{"D"}};
  root->addChild(a);
  root->addChild(e);
  a->addChild(b);
  e->addChild(d);

  return root;
}

ExtendedVariableOrder* testCreate2() {
  ExtendedVariableOrderNode* l{new ExtendedVariableOrderNode{"Location"}};
  ExtendedVariableOrderNode* c{new ExtendedVariableOrderNode{"Competitor", {l->getName()}}};
  ExtendedVariableOrderNode* p{new ExtendedVariableOrderNode{"Product", {l->getName()}}};
  ExtendedVariableOrderNode* i{new ExtendedVariableOrderNode{"Inventory", {l->getName(), p->getName()}}};
  ExtendedVariableOrderNode* s{new ExtendedVariableOrderNode{"Sale", {p->getName()}}};
  ExtendedVariableOrderLeaf* b{
      new ExtendedVariableOrderLeaf{"Branch", {l->getName(), p->getName(), i->getName()}}};
  ExtendedVariableOrderLeaf* cn{new ExtendedVariableOrderLeaf{"Competition", {l->getName(), c->getName()}}};
  ExtendedVariableOrderLeaf* ss{new ExtendedVariableOrderLeaf{"Sales", {p->getName(), s->getName()}}};
  // c.addKey(l.getName());
  // p.addKey(l.getName());
  // i.addKey(l.getName());
  // i.addKey(p.getName());
  // s.addKey(p.getName());
  s->addChild(ss);
  i->addChild(b);
  c->addChild(cn);
  p->addChild(s);
  p->addChild(i);
  l->addChild(c);
  l->addChild(p);

  return l;
}

void recursiveTest(const ExtendedVariableOrder& root) {
  std::cout << root.getName() << ' ' << root.isLeaf() << '\n';
  if (!root.isLeaf()) {
    for (const ExtendedVariableOrder* x :
         static_cast<const ExtendedVariableOrderNode*>(&root)->getChildren()) {
      recursiveTest(*x);
    }
  }
}

int main() {
  std::cout << std::boolalpha;
  ExtendedVariableOrder* testV{testCreate()};
  recursiveTest(*testV);
  std::cout << '\n';

  std::cout << factorizeSQL(*testV) << '\n';
  std::cout << '\n';
  delete testV;

  ExtendedVariableOrder* testV2{testCreate2()};
  recursiveTest(*testV2);
  std::cout << '\n';

  std::cout << factorizeSQL(*testV2) << '\n';
  std::cout << '\n';
  delete testV2;

  return 0;
}