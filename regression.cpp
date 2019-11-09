#include <vector>
#include <unordered_set>
#include <string>
#include <iostream>
#include <cassert>
#include <cmath>
#include "variableOrder.h"
#include "regression.h"

typedef std::string sql;

/**
 * code based on "Factorized Databases" by Dan Olteanu, Maximilian Schleich (Figure 5)
 * URL: https://doi.org/10.14778/3007263.3007312
 **/
void factorizeSQL(const ExtendedVariableOrder& varOrder, pqxx::work& transaction) {
  const sql& name{varOrder.getName()};
  if (varOrder.isLeaf()) {
    transaction.exec("CREATE TABLE " + name + "type(" + name + "n varchar(50)," + name +
                     "d int);\nINSERT INTO " + name + "type VALUES ('" + name + "', 0);\n");

    /* createTablesFile << "CREATE TABLE " << name << "type(" << name << "n varchar(50)," << name
               << "d int);\nINSERT INTO " << name << "type VALUES ('" << name << "', 0);\n"; */

    sql deg = name + "d AS deg", lineage = "CONCAT('(', " + name + "n, ',', " + name + "d, ')') AS lineage",
        agg = "1 AS agg";

    sql schema{""};
    for (const sql& x : varOrder.getKey()) {
      schema += name + "." + x + ", ";
    }

    transaction.exec("CREATE VIEW Q" + name + " AS (SELECT " + schema + lineage + ", " + deg + ", " + agg +
                     " FROM " + name + ", " + name + "type);\n");

  } else {
    transaction.exec("CREATE TABLE " + name + "type(" + name + "n varchar(50), " + name + "d int);\n");
    /* createTablesFile << "CREATE TABLE " << name << "type(" << name << "n varchar(50), " << name
                     << "d int);\n"; */

    const int d = 1; // linear
    for (int i{0}; i <= 2 * d; ++i) {
      transaction.exec("INSERT INTO " + name + "type VALUES('" + name + "', " + std::to_string(i) + ");\n");
    }

    // std::vector<sql> retChild;
    // retChild.reserve(varOrder.getChildren().size());
    sql join{""}, lineage{"CONCAT('(', "}, deg{""},
        agg{"SUM(POWER(Q" + varOrder.getChildren().front().getName() + "." + name + "," + name + "d)"};

    // ++id;

    sql lastName{""};
    for (const ExtendedVariableOrder& x : varOrder.getChildren()) {
      // retChild.push_back(factorizeSQL(x, id));
      // join += factorizeSQL(x, id, createTablesFile) + ", ";
      // ret += factorizeSQL(x, createTablesFile);
      factorizeSQL(x, transaction);

      const sql xName{x.getName()};
      if (lastName != "") {
        join += " JOIN ";
      }
      join += "Q" + xName;
      if (lastName != "") {
        join += " ON Q" + lastName + "." + name + "=Q" + xName + "." + name;
      }

      deg += "Q" + xName + ".deg + ";
      lineage += "Q" + xName + ".lineage, ',', ";
      agg += " * Q" + xName + ".agg";

      lastName = xName;
    }
    if (lastName != "") {
      join += ", ";
    }

    deg += name + "d";
    lineage += name + "n, ',', " + name + "d, ')')";
    agg += ")";

    sql key{""};
    for (const sql& x : varOrder.getKey()) {
      key += x + ", ";
    }

    transaction.exec("CREATE VIEW Q" + name + " AS (SELECT " + key + " " + lineage + " AS lineage, " + deg +
                     " AS deg, " + agg + " AS agg FROM " + join + name + "type WHERE " + deg +
                     " <= " + std::to_string(2 * d) + " GROUP BY " + key + lineage + ", " + deg + ");\n");
  }
}

void factorizeSQL(const ExtendedVariableOrder& varOrder, pqxx::connection& c) {
  pqxx::work transaction{c};
  // special case for root/intercept to avoid redundancy and allow simpler varOrders
  const sql& name{varOrder.getName()};
  const int d = 1; // linear

  assert(!varOrder.isLeaf());

  // process all children (usually just one)
  sql join{""}, lineage{"CONCAT('(', "}, deg{""}, agg{"SUM("};
  bool first{true};
  for (const ExtendedVariableOrder& x : varOrder.getChildren()) {
    factorizeSQL(x, transaction);

    const sql& xName{x.getName()};
    if (first) {
      first = false;
      join += "Q" + xName;
      deg += "Q" + xName + ".deg";
      lineage += "Q" + xName + ".lineage";
      agg += "Q" + xName + ".agg";

    } else {
      join += ", Q" + xName;
      deg += " + Q" + xName + ".deg";
      lineage += ", ',', Q" + xName + ".lineage";
      agg += " * Q" + xName + ".agg";
    }
  }

  lineage += ", ')')";
  agg += ")";

  // combine the results
  transaction.exec("CREATE VIEW Q" + name + " AS (SELECT " + lineage + " AS lineage, " + deg + " AS deg, " +
                   agg + " AS agg FROM " + join + " WHERE " + deg + " <= " + std::to_string(2 * d) +
                   " GROUP BY " + lineage + ", " + deg + ");\n");

  transaction.commit();
}

void fillMatrix(const std::vector<std::string>& relevantColumns, pqxx::connection& c,
                std::vector<std::vector<int>>& cofactorMatrix) {
  assert(relevantColumns.size() > 1);
  const size_t nrElems{relevantColumns.size()};
  for (auto& x : cofactorMatrix) {
    x.reserve(nrElems);
  }

  pqxx::nontransaction n{c};
  for (size_t i{0}; i < nrElems; ++i) {
    // diagonal element
    const std::string& iName{relevantColumns.at(i)};
    auto res{n.exec("SELECT agg FROM QT WHERE lineage LIKE '%" + iName + ",2%';")};
    assert(res.size() == 1);
    assert(res[0].size() == 1);
    cofactorMatrix.at(i).push_back(res[0][0].as<int>());

    for (size_t j{i + 1}; j < nrElems; ++j) {
      // i,j and j,i
      res = n.exec("SELECT agg FROM QT WHERE lineage LIKE '%" + iName + ",1%' AND lineage LIKE '%" +
                   relevantColumns.at(j) + ",1%';");
      // std::cerr << i << ", " << j << "\n";
      assert(res.size() == 1);
      assert(res[0].size() == 1);
      cofactorMatrix.at(i).push_back(res[0][0].as<int>());
      cofactorMatrix.at(j).push_back(res[0][0].as<int>());
    }
    assert(cofactorMatrix.size() == cofactorMatrix.at(i).size());
  }
}

std::string stringOfVector(const std::vector<int>& array) {
  std::string out{"[ "};

  for (const int elem : array) {
    out += std::to_string(elem) + " | ";
  }

  // remove trailing " | "
  if (out.size() > 2) {
    out.erase(out.size() - 3, 3);
  }

  out += "]";
  return out;
}

/**
 * assumes label being at index 0 of relevantColumns
 *
 * code based on "Factorized Databases" by Dan Olteanu, Maximilian Schleich (Chapter 4)
 * URL: https://doi.org/10.14778/3007263.3007312
 **/
std::vector<double> batchGradientDescent(const std::vector<std::string>& relevantColumns,
                                         pqxx::connection& c) {
  std::vector<std::vector<int>> cofactorMatrix(relevantColumns.size(), std::vector<int>{});
  fillMatrix(relevantColumns, c, cofactorMatrix);

  for (const auto& x : cofactorMatrix) {
    std::cout << stringOfVector(x) << '\n';
  }

  const size_t n{relevantColumns.size()};

  // start with some initial value
  std::vector<double> theta(n, 1);
  // label is fixed with -1
  theta.at(0) = -1.;

  // TODO: find good values
  double alpha{0.003};
  const double lambda{0.003};

  // repeat until error is sufficiently small
  const double eps{1e-10};
  bool notExact{true};

  std::vector<double> epsilon(n, INFINITY);
  while (notExact) {
    notExact = false;

    // compute new epsilon for all features
    for (size_t j{1}; j < n; ++j) {
      double epsilonNew{0.};
      for (size_t k{0}; k < n; ++k) {
        epsilonNew += theta.at(k) * cofactorMatrix.at(k).at(j);
      }

      // using ridge regularization term derived by theta_j
      epsilonNew += lambda * 2 * theta.at(j);

      epsilonNew *= alpha;

      // not exact enough
      if (std::fabs(epsilonNew) > eps) {
        notExact = true;

        // alpha needs adjusting
        if (std::fabs(epsilonNew / 2) >= std::fabs(epsilon.at(j)) || std::fabs(epsilonNew) > 1e6) {
          // alpha = std::min(alpha / 3, std::fabs(epsilon.at(j) / epsilonNew));
          alpha /= 3;
          epsilonNew /= 3;
        }
      }

      // epsilonNew *= alpha;

      epsilon.at(j) = epsilonNew;
    }

    for (size_t j{1}; j < n; ++j) {
      theta.at(j) -= epsilon.at(j);
    }
  }

  return theta;
}

/* sql factorizeSQL(const ExtendedVariableOrder& varOrder) {
  int id{0};
  return factorizeSQL(varOrder, id);
} */
