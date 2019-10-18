#include <vector>
#include <unordered_set>
#include <string>
#include <iostream>
#include <fstream>
#include "variableOrder.h"
#include "regression.h"

// version for Sql Server Management Studio (possibly outdated)

/**
 * code based on "Factorized Databases" by Dan Olteanu, Maximilian Schleich (Figure 5)
 * URL: https://doi.org/10.14778/3007263.3007312
 **/
sql factorizeSQL(const ExtendedVariableOrder& varOrder, std::ofstream& createTablesFile) {
  sql ret{""};
  const sql& name{varOrder.getName()};
  if (varOrder.isLeaf()) {
    createTablesFile << "CREATE TABLE " << name << "type(" << name << "n varchar(50)," << name
                     << "d int);\nINSERT INTO " << name << "type VALUES ('" << name << "', 0);\n";

    sql deg = name + "d AS deg", lineage = "CONCAT('(', " + name + "n, ',', " + name + "d, ')') AS lineage",
        agg = "1 AS agg";

    sql schema{""};
    for (const sql& x : varOrder.getKey()) {
      schema += name + "." + x + ", ";
    }

    ret += "GO\nCREATE VIEW Q" + name + " AS (SELECT " + schema + lineage + ", " + deg + ", " + agg +
           " FROM " + name + ", " + name + "type);\n";

  } else {
    createTablesFile << "CREATE TABLE " << name << "type(" << name << "n varchar(50), " << name
                     << "d int);\n";

    int d = 1; // linear
    for (int i{0}; i <= 2 * d; ++i) {
      createTablesFile << "INSERT INTO " << name << "type VALUES('" << name << "', " << i << ");\n";
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
      ret += factorizeSQL(x, createTablesFile);

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

    ret += "GO\nCREATE VIEW Q" + name + " AS (SELECT " + key + " " + lineage + " AS lineage, " + deg +
           " AS deg, " + agg + " AS agg FROM " + join + name + "type WHERE " + deg +
           " <= " + std::to_string(2 * d) + " GROUP BY " + key + lineage + ", " + deg + ");\n";
  }

  return ret;
}

sql factorizeSQL(const ExtendedVariableOrder& varOrder) {
  // int id{0};
  std::ofstream createTablesFile("createTables.sql");
  sql ret{factorizeSQL(varOrder, createTablesFile)};
  createTablesFile.close();
  return ret;
}
