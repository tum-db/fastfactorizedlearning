#include <pqxx/pqxx>
#include <vector>
#include <string>
#include <iostream>
#include <cassert>
#include <cmath>
#include <algorithm>
#include "variableOrder.h"
#include "regression.h"

typedef std::string sql;

/**
 * applies feature scaling to all features in relevantColumns
 *
 * assumes label being at index 0 of relevantColumns
 * and intercept being at last index of relevantColumns
 *
 * leaves need to contain at least all Nodes representing tables which contain a relevant column
 * e.g. if tables "store" and "sale" both have column "date" (and "date" is a feature or label)
 * both of them should be included in leaves
 **/
std::vector<scaleFactors> scaleFeatures(const std::vector<std::string>& relevantColumns,
                                        std::vector<ExtendedVariableOrder*>& leaves, pqxx::connection& c) {
  size_t n{relevantColumns.size() - 1};
  assert(n > 2);

  // find out which columns appear in which table(s)
  std::vector<std::vector<size_t>> relevantTables(n, std::vector<size_t>{});

  // columns that need converting
  std::vector<std::vector<size_t>> convertCols(leaves.size(), std::vector<size_t>{});

  for (size_t i{0}; i < leaves.size(); ++i) {
    assert(leaves.at(i)->isLeaf());
    // convertCols.at(i).reserve(leaves.at(i)->getKey().size());

    const std::vector<std::string>& keys{leaves.at(i)->getKey()};
    for (const auto& col : keys) {
      for (size_t j{0}; j < n; ++j) {
        // column j appears in table i
        if (col == relevantColumns.at(j)) {
          relevantTables.at(j).push_back(i);

          // table needs changing
          if (j > 0) {
            convertCols.at(i).push_back(j);
          }

          // other columns can't match
          break;
        }
      }
    }
  }

  pqxx::work transaction{c};
  std::vector<scaleFactors> aggregates;
  aggregates.reserve(n + 1);

  // compute the required aggregates
  for (size_t i{0}; i < n; ++i) {
    const sql& column{relevantColumns.at(i)};

    sql query{""};
    bool first{true};
    for (const size_t j : relevantTables.at(i)) {
      if (first) {
        first = false;
        query = "WITH unionOfAllTables AS (SELECT " + column + " FROM " + leaves.at(j)->getName();
      } else {
        query += " UNION SELECT " + column + " FROM " + leaves.at(j)->getName();
      }
    }
    query += ") ";

    const sql select{"SELECT AVG(" + column + ") as avg, MAX(ABS(" + column +
                     ")) AS max FROM unionOfAllTables;"};

    auto res{transaction.exec(query + select)};
    assert(res.size() == 1);
    assert(res[0].size() == 2);

    // don't scale if no value is distinct from null
    if (res[0][0].is_null()) {
      aggregates.push_back({0, 1});
    } else {
      aggregates.push_back({res[0][0].as<double>(), res[0][1].as<double>()});
    }

    // don't scale if it's already good
    if (aggregates.at(i).max == 0. || (aggregates.at(i).max < 5 && aggregates.at(i).max > 0.1)) {
      aggregates.at(i) = {0., 1.};
      for (const size_t j : relevantTables.at(i)) {
        convertCols.at(j).erase(std::remove(convertCols.at(j).begin(), convertCols.at(j).end(), i),
                                convertCols.at(j).end());
      }

      // } else {
      // // column i appears in table j
      // for (const size_t j : relevantTables.at(i)) {
      //   // table needs changing
      //   if (i > 0) {
      //     convertCols.at(j).push_back(i);
      //   }
      // }
    }
  }

  // compute new tables if necessary
  for (size_t i{0}; i < leaves.size(); ++i) {
    if (convertCols.at(i).size() == 0) {
      continue;
    }

    const sql orig{leaves.at(i)->getName()};
    leaves.at(i)->convertName({});
    // CREATE converted table
    sql query{"CREATE VIEW " + leaves.at(i)->getName() + " AS SELECT "};

    const std::vector<std::string>& keys{leaves.at(i)->getKey()};
    assert(keys.size() >= convertCols.at(i).size());

    const std::string* curCol{&relevantColumns.at(convertCols.at(i).at(0))};
    for (size_t j{0}, k{0}; j < keys.size(); ++j) {
      if (j > 0) {
        query += ", ";
      }

      // column remains unchanged
      if (curCol == NULL || *curCol != keys.at(j)) {
        query += keys.at(j);

        // perform scaling
      } else {
        // xnew = (x - avg) / max
        query += "((" + *curCol + " - " + std::to_string(aggregates.at(convertCols.at(i).at(k)).avg) +
                 ") / " + std::to_string(aggregates.at(convertCols.at(i).at(k)).max) + ")::real AS " +
                 *curCol;

        // move to next column that requires change
        if (++k >= convertCols.at(i).size()) {
          curCol = NULL;
        } else {
          curCol = &relevantColumns.at(convertCols.at(i).at(k));
        }
      }
    }

    transaction.exec(query + " FROM " + orig);
  }

  transaction.commit();

  return aggregates;
}

/**
 * code based on "Factorized Databases" by Dan Olteanu, Maximilian Schleich (Figure 5)
 * URL: https://doi.org/10.14778/3007263.3007312
 **/
void factorizeSQL(const ExtendedVariableOrder& varOrder, pqxx::work& transaction) {
  const sql& name{varOrder.getName()};
  if (varOrder.isLeaf()) {
    transaction.exec("CREATE TABLE " + name + "_type(" + name + "_n varchar(50)," + name +
                     "_d int);\nINSERT INTO " + name + "_type VALUES ('" + name + "', 0);\n");

    /* createTablesFile << "CREATE TABLE " << name << "type(" << name << "n varchar(50)," << name
               << "d int);\nINSERT INTO " << name << "type VALUES ('" << name << "', 0);\n"; */

    sql deg = name + "_d AS " + name + "_deg", lineage = "''AS " + name + "_lineage",
        agg = "1 AS " + name + "_agg";

    sql schema{""};
    for (const sql& x : varOrder.getKey()) {
      schema += name + "." + x + ", ";
    }

    transaction.exec("CREATE VIEW Q" + name + " AS (SELECT " + schema + lineage + ", " + deg + ", " + agg +
                     " FROM " + name + ", " + name + "_type);\n");

  } else {
    transaction.exec("CREATE TABLE " + name + "_type(" + name + "_n varchar(50), " + name + "_d int);\n");
    /* createTablesFile << "CREATE TABLE " << name << "type(" << name << "n varchar(50), " << name
                     << "d int);\n"; */

    const int d = 1; // linear
    for (int i{0}; i <= 2 * d; ++i) {
      transaction.exec("INSERT INTO " + name + "_type VALUES('" + name + "', " + std::to_string(i) + ");\n");
    }

    // std::vector<sql> retChild;
    // retChild.reserve(varOrder.getChildren().size());
    sql join{""}, lineage{"CONCAT("}, deg{""}, agg;

    // categorical variables have to be treated differently => grouping instead of POWER
    if (varOrder.isCategorical()) {
      agg = "SUM(1";
    } else {
      agg = "SUM(POWER(COALESCE(Q" + varOrder.getChildren().front().getName() + "." + name + ",0)," + name +
            "_d)";
    }

    // ++id;

    sql lastName{""};
    // construct queries for all children and prepare statements for this node's query
    for (const ExtendedVariableOrder& x : varOrder.getChildren()) {
      // retChild.push_back(factorizeSQL(x, id));
      // join += factorizeSQL(x, id, createTablesFile) + ", ";
      // ret += factorizeSQL(x, createTablesFile);
      factorizeSQL(x, transaction);

      const sql xName{x.getName()};
      if (lastName != "") {
        join += " NATURAL JOIN ";
      }
      join += "Q" + xName;
      // if (lastName != "") {
      //   join += " ON Q" + lastName + "." + name + "=Q" + xName + "." + name;
      // }

      deg += "Q" + xName + "." + xName + "_deg + ";
      lineage += "Q" + xName + "." + xName + "_lineage,";
      agg += " * Q" + xName + "." + xName + "_agg";

      lastName = xName;
    }
    if (lastName != "") {
      join += ", ";
    }

    deg += name + "_d";
    lineage += "CASE WHEN " + name + "_d > 0 THEN CONCAT('(', " + name + "_n, ',', " + name +
               "_d, ')') ELSE '' END)";
    agg += ")";

    sql key{""};
    for (const sql& x : varOrder.getKey()) {
      key += x + ", ";
    }

    // categorical variables have to be treated differently => grouping instead of POWER
    sql cat{""};
    if (varOrder.isCategorical()) {
      cat = "Q" + varOrder.getChildren().front().getName() + "." + name + ", ";
    }

    transaction.exec("CREATE VIEW Q" + name + " AS (SELECT " + key + " " + lineage + " AS " + name +
                     "_lineage, " + deg + " AS " + name + "_deg, " + agg + " AS " + name + "_agg FROM " +
                     join + name + "_type WHERE " + deg + " <= " + std::to_string(2 * d) + " GROUP BY " +
                     cat + key + lineage + ", " + deg + ");\n");
  }
}

void factorizeSQL(const ExtendedVariableOrder& varOrder, pqxx::connection& c) {
  pqxx::work transaction{c};
  // special case for root/intercept to avoid redundancy and allow simpler varOrders
  const sql& name{varOrder.getName()};
  const int d = 1; // linear

  assert(!varOrder.isLeaf());

  // process all children (usually just one)
  sql join{""}, lineage{"CONCAT("}, deg{""}, agg{"SUM("};
  bool first{true};
  for (const ExtendedVariableOrder& x : varOrder.getChildren()) {
    factorizeSQL(x, transaction);

    const sql& xName{x.getName()};
    if (first) {
      first = false;
      join += "Q" + xName;
      deg += "Q" + xName + "." + xName + "_deg";
      lineage += "Q" + xName + "." + xName + "_lineage";
      agg += "Q" + xName + "." + xName + "_agg";

    } else {
      join += ", Q" + xName;
      deg += " + Q" + xName + "." + xName + "_deg";
      lineage += ", ',', Q" + xName + "." + xName + "_lineage";
      agg += " * Q" + xName + "." + xName + "_agg";
    }
  }

  lineage += ")";
  agg += ")";

  // combine the results
  transaction.exec("CREATE TABLE Q" + name + " AS (SELECT " + lineage + " AS lineage, " + deg + " AS deg, " +
                   agg + " AS agg FROM " + join + " WHERE " + deg + " <= " + std::to_string(2 * d) +
                   " GROUP BY " + lineage + ", " + deg + ");\n");

  transaction.commit();
}

void fillMatrix(const std::vector<std::string>& relevantColumns, pqxx::connection& c,
                std::vector<std::vector<double>>& cofactorMatrix) {
  assert(relevantColumns.size() > 1);
  const size_t nrElems{relevantColumns.size() - 1};
  for (auto& x : cofactorMatrix) {
    x.reserve(nrElems + 1);
  }

  const std::string& intercept{relevantColumns.back()};

  pqxx::nontransaction n{c};
  for (size_t i{0}; i < nrElems; ++i) {
    // diagonal element
    const std::string& iName{relevantColumns.at(i)};
    auto res{n.exec("SELECT agg FROM Q" + intercept + " WHERE lineage LIKE '%(" + iName + ",2)%';")};
    assert(res.size() == 1);
    assert(res[0].size() == 1);
    cofactorMatrix.at(i).push_back(res[0][0].as<double>());

    for (size_t j{i + 1}; j < nrElems; ++j) {
      // i,j and j,i
      res = n.exec("SELECT agg FROM Q" + intercept + " WHERE lineage LIKE '%(" + iName +
                   ",1)%' AND lineage LIKE '%(" + relevantColumns.at(j) + ",1)%';");
      // std::cerr << i << ", " << j << "\n";
      assert(res.size() == 1);
      assert(res[0].size() == 1);
      cofactorMatrix.at(i).push_back(res[0][0].as<double>());
      cofactorMatrix.at(j).push_back(res[0][0].as<double>());

      // std::cout << "Filled:" << i << ", " << j << '\n';
    }

    // intercept
    // i,n and n,i
    res = n.exec("SELECT agg FROM Q" + intercept + " WHERE lineage LIKE '%(" + iName + ",1)%' AND deg = 1;");
    assert(res.size() == 1);
    assert(res[0].size() == 1);
    cofactorMatrix.at(i).push_back(res[0][0].as<double>());
    cofactorMatrix.at(nrElems).push_back(res[0][0].as<double>());

    assert(cofactorMatrix.size() == cofactorMatrix.at(i).size());
  }

  // intercept
  // n,n
  auto res{n.exec("SELECT agg FROM Q" + intercept + " WHERE deg = 0;")};
  assert(res.size() == 1);
  assert(res[0].size() == 1);
  cofactorMatrix.at(nrElems).push_back(res[0][0].as<double>());
}

std::string stringOfVector(const std::vector<long>& array) {
  std::string out{"[ "};

  for (const long elem : array) {
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
 * and intercept being at last index of relevantColumns
 *
 * code based on "Factorized Databases" by Dan Olteanu, Maximilian Schleich (Chapter 4)
 * URL: https://doi.org/10.14778/3007263.3007312
 **/
std::vector<double> batchGradientDescent(const std::vector<std::string>& relevantColumns,
                                         pqxx::connection& c) {
  std::vector<std::vector<double>> cofactorMatrix(relevantColumns.size(), std::vector<double>{});
  fillMatrix(relevantColumns, c, cofactorMatrix);

  // for (const auto& x : cofactorMatrix) {
  //   std::cout << stringOfVector(x) << '\n';
  // }

  const size_t n{relevantColumns.size()};

  // start with some initial value
  std::vector<double> theta(n, 1.);
  // label is fixed with -1
  theta.at(0) = -1;

  // TODO: find good values
  double alpha{0.003};
  const double lambda{0.003};

  // repeat until error is sufficiently small
  const double eps{1e-6};
  const double abortAlpha{1e-15};
  bool notExact{true};

  std::vector<double> epsilon(n, INFINITY);
  int i{0};
  while (notExact) {
    if (++i > 100000000) {
      std::cout << "Aborted after i=" << i << " iterations with alpha=" << alpha << '\n';
      break;
      // } else if (i % 1000000 == 0) {
      //   std::cout << i << '\n';
    }
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
        if (std::fabs(epsilonNew / 2) >= std::fabs(epsilon.at(j)) || std::fabs(epsilonNew) > 1e4) {
          // alpha = std::min(alpha / 3, std::fabs(epsilon.at(j) / epsilonNew));
          alpha /= 3;
          epsilonNew /= 3;
          if (alpha < abortAlpha) {
            break;
          }
        }
      }

      // epsilonNew *= alpha;

      epsilon.at(j) = epsilonNew;
    }

    if (alpha < abortAlpha) {
      std::cout << "Aborted after i=" << i << " iterations with alpha=" << alpha << '\n';
      break;
    }

    for (size_t j{1}; j < n; ++j) {
      theta.at(j) -= epsilon.at(j);
    }
  }

  // std::cout << "Finished after i=" << i << " iterations with alpha=" << alpha << '\n';

  return theta;
}

/**
 * uses feature scaling, factorize-SQL and batch gradient descent to return values for theta
 *
 * assumes label being at index 0 of relevantColumns
 * and intercept being at last index of relevantColumns
 *
 * stores average value of the label in avg
 *
 */
std::vector<double> linearRegression(ExtendedVariableOrder& varOrder,
                                     const std::vector<std::string>& relevantColumns, pqxx::connection& c,
                                     double& avg) {
  std::vector<ExtendedVariableOrder*> leaves;
  varOrder.findLeaves(leaves);

  std::vector<scaleFactors> scaleAggs{scaleFeatures(relevantColumns, leaves, c)};
  // std::cout << "Feature Scaling complete\n";

  factorizeSQL(varOrder, c);
  // std::cout << "Creation of tables and views complete.\n\n";

  std::vector<double> theta{batchGradientDescent(relevantColumns, c)};
  assert(theta.size() == relevantColumns.size());
  // std::cout << "Batch Gradient descent complete.\n";
  // std::cout << stringOfVector(theta) << '\n';

  // for the constant term
  double sum{0.};

  // scale result
  assert(theta.size() == scaleAggs.size() + 1);
  for (size_t i{1}; i < scaleAggs.size(); ++i) {
    theta.at(i) /= scaleAggs.at(i).max;

    sum += theta.at(i) * scaleAggs.at(i).avg;
  }

  theta.back() = scaleAggs.front().avg - sum;

  // std::cout << stringOfVector(theta) << '\n';

  avg = scaleAggs.front().avg;

  return theta;
}

std::vector<double> naiveBGD(const std::vector<std::string>& relevantColumns, pqxx::connection& c) {
  const size_t n{relevantColumns.size()};

  // start with some initial value
  std::vector<double> theta(n, 1.);
  // label is fixed with -1
  theta.at(0) = -1;

  // TODO: find good values
  double alpha{0.003};
  const double lambda{0.003};

  // repeat until error is sufficiently small
  const double eps{1e-6};
  const double abortAlpha{1e-10};
  bool notExact{true};

  std::vector<double> epsilon(n, INFINITY);
  int i{0};
  while (notExact) {
    if (++i > 100000000) {
      std::cout << "Aborted after i=" << i << " iterations with alpha=" << alpha << '\n';
      break;
    }
    notExact = false;

    // compute new epsilon for all features
    for (size_t j{1}; j < n; ++j) {

      sql epsilonQuery{"SELECT SUM(("};

      for (size_t k{0}; k < n - 1; ++k) {
        epsilonQuery += std::to_string(theta.at(k)) + "*" + relevantColumns.at(k) + " + ";
      }
      epsilonQuery += std::to_string(theta.back()) + ")*" + ((j==n-1) ? "1" : relevantColumns.at(j)) + ") FROM joinView";

      // execute the Query
      pqxx::nontransaction transaction{c};
      auto res{transaction.exec(epsilonQuery)};
      assert(res.size() == 1);
      assert(res[0].size() == 1);
      double epsilonNew{res[0][0].as<double>()};

      // using ridge regularization term derived by theta_j
      epsilonNew += lambda * 2 * theta.at(j);

      epsilonNew *= alpha;

      // not exact enough
      if (std::fabs(epsilonNew) > eps) {
        notExact = true;

        // alpha needs adjusting
        if (std::fabs(epsilonNew / 2) >= std::fabs(epsilon.at(j)) || std::fabs(epsilonNew) > 1e4) {
          alpha /= 3;
          epsilonNew /= 3;
          if (alpha < abortAlpha) {
            break;
          }
        }
      }

      epsilon.at(j) = epsilonNew;
    }

    if (alpha < abortAlpha) {
      std::cout << "Aborted after i=" << i << " iterations with alpha=" << alpha << '\n';
      break;
    }

    // update theta
    for (size_t j{1}; j < n; ++j) {
      theta.at(j) -= epsilon.at(j);
    }
  }

  // std::cout << "Finished after i=" << i << " iterations with alpha=" << alpha << '\n';

  return theta;
}

std::vector<double> naiveRegression(ExtendedVariableOrder& varOrder,
                                    const std::vector<std::string>& relevantColumns, pqxx::connection& c,
                                    double& avg) {
  std::vector<ExtendedVariableOrder*> leaves;
  varOrder.findLeaves(leaves);

  std::vector<scaleFactors> scaleAggs{scaleFeatures(relevantColumns, leaves, c)};
  // std::cout << "Feature Scaling complete\n";

  pqxx::work transaction{c};

  transaction.exec("DROP VIEW IF EXISTS joinView;");

  sql join{"CREATE VIEW joinView AS (SELECT* FROM "};

  bool first{true};
  for (const auto leaf : leaves) {
    if (first) {
      first = false;
      join += leaf->getName();
    } else {
      join += " NATURAL JOIN " + leaf->getName();
    }
  }

  transaction.exec(join + ")");
  transaction.commit();

  std::vector<double> theta{naiveBGD(relevantColumns, c)};
  // std::cout << stringOfVector(theta) << '\n';

  // for the constant term
  double sum{0.};

  // scale result
  assert(theta.size() == scaleAggs.size() + 1);
  for (size_t i{1}; i < scaleAggs.size(); ++i) {
    theta.at(i) /= scaleAggs.at(i).max;

    sum += theta.at(i) * scaleAggs.at(i).avg;
  }

  theta.back() = scaleAggs.front().avg - sum;

  // std::cout << stringOfVector(theta) << '\n';

  avg = scaleAggs.front().avg;

  return theta;
}