#include <vector>
#include <unordered_set>
#include <string>
#include <iostream>
#include <pqxx/pqxx>
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
  ExtendedVariableOrder t{"T"};
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
  t.addChild(l);

  return t;
}

void recursiveTest(const ExtendedVariableOrder& root) {
  std::cout << root.getName() << ' ' << root.isLeaf() << '\n';
  for (const ExtendedVariableOrder& x : root.getChildren()) {
    recursiveTest(x);
  }
}

void dropAll(pqxx::connection& c) {
  pqxx::work w{c};
  w.exec("\
    DROP TABLE IF EXISTS Branchtype CASCADE;\
    DROP TABLE IF EXISTS Competitiontype CASCADE;\
    DROP TABLE IF EXISTS Competitortype CASCADE;\
    DROP TABLE IF EXISTS Inventorytype CASCADE;\
    DROP TABLE IF EXISTS Locationtype CASCADE;\
    DROP TABLE IF EXISTS Producttype CASCADE;\
    DROP TABLE IF EXISTS Salestype CASCADE;\
    DROP TABLE IF EXISTS Saletype CASCADE;\
    DROP VIEW IF EXISTS QBranch CASCADE;\
    DROP VIEW IF EXISTS QCompetition CASCADE;\
    DROP VIEW IF EXISTS QCompetitor CASCADE;\
    DROP VIEW IF EXISTS QInventory CASCADE;\
    DROP VIEW IF EXISTS QLocation CASCADE;\
    DROP VIEW IF EXISTS QProduct CASCADE;\
    DROP VIEW IF EXISTS QSale CASCADE;\
    DROP VIEW IF EXISTS QSales CASCADE;\
    DROP VIEW IF EXISTS QT CASCADE;");
  w.commit();
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
  std::cout << '\n';

  // std::cout << factorizeSQL(testV2) << '\n';
  // std::cout << '\n';

  try {
    pqxx::connection c{"dbname=SalesWithNumbers hostaddr=127.0.0.1 port=5433"};
    if (c.is_open()) {
      std::cout << "Connected to: " << c.dbname() << '\n';

      dropAll(c);
      std::cout << "Dropped all tables and views.\n";
      // std::cin.ignore();

      factorizeSQL(testV2, c);
      std::cout << "Creation of tables and views complete.\n";

      c.disconnect();
    } else {
      std::cout << "Failed to connect!\n";
    }

  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
  }

  return 0;
}