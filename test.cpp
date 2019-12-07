#include <pqxx/pqxx>
#include <vector>
#include <string>
#include <iostream>
#include <cassert>
#include <cmath>
#include <random>
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

ExtendedVariableOrder createSales() {
  ExtendedVariableOrder t{"T"};
  ExtendedVariableOrder l{"Location"};
  ExtendedVariableOrder c{"Competitor", {l.getName()}};
  ExtendedVariableOrder p{"Product", {l.getName()}, true};
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

ExtendedVariableOrder createFavorita() {
  ExtendedVariableOrder t{"t"};

  ExtendedVariableOrder items{"items", {"item_nbr", "family", "class", "perishable"}};
  ExtendedVariableOrder family{"family", {"item_nbr", "class", "perishable"}, true};
  ExtendedVariableOrder clas{"class", {"item_nbr", "perishable"}};
  ExtendedVariableOrder perishable{"perishable", {"item_nbr"}};
  ExtendedVariableOrder item_nbr{"item_nbr", {"date", "store_nbr"}};

  ExtendedVariableOrder holidays_events{
      "holidays_events_conv", {"date", "type", "locale", "locale_name", "description", "transferred"}};
  ExtendedVariableOrder type{"type", {"date", "locale", "locale_name", "description", "transferred"}, true};
  ExtendedVariableOrder locale{"locale", {"date", "locale_name", "description", "transferred"}, true};
  ExtendedVariableOrder locale_name{"locale_name", {"date", "description", "transferred"}, true};
  ExtendedVariableOrder description{"description", {"date", "transferred"}, true};
  ExtendedVariableOrder transferred{"transferred", {"date"}, true};
  ExtendedVariableOrder date{"date", {"store_nbr"}};

  ExtendedVariableOrder oil{"oil_conv", {"date", "dcoilwtico"}};
  ExtendedVariableOrder dcoilwtico{"dcoilwtico", {"date"}};

  ExtendedVariableOrder transactions{"transactions_conv", {"date", "store_nbr", "txns"}};
  ExtendedVariableOrder txns{"txns", {"date", "store_nbr"}};
  ExtendedVariableOrder store_nbr{"store_nbr"};

  ExtendedVariableOrder stores{"stores", {"store_nbr", "city", "state", "stype", "cluster"}};
  ExtendedVariableOrder city{"city", {"store_nbr", "state", "stype", "cluster"}, true};
  ExtendedVariableOrder state{"state", {"store_nbr", "stype", "cluster"}, true};
  ExtendedVariableOrder stype{"stype", {"store_nbr", "cluster"}, true};
  ExtendedVariableOrder cluster{"cluster", {"store_nbr"}};

  ExtendedVariableOrder train{"train_conv",
                              {"id", "date", "store_nbr", "item_nbr", "unit_sales", "onpromotion"}};
  ExtendedVariableOrder onpromotion{"onpromotion", {"id", "date", "store_nbr", "item_nbr", "unit_sales"}};
  ExtendedVariableOrder unit_sales{"unit_sales", {"id", "date", "store_nbr", "item_nbr"}};
  ExtendedVariableOrder id{"id", {"date", "store_nbr", "item_nbr"}};

  city.addChild(stores);
  state.addChild(city);
  stype.addChild(state);
  cluster.addChild(stype);
  store_nbr.addChild(cluster);

  txns.addChild(transactions);
  date.addChild(txns);

  dcoilwtico.addChild(oil);
  date.addChild(dcoilwtico);

  onpromotion.addChild(train);
  unit_sales.addChild(onpromotion);
  id.addChild(unit_sales);
  item_nbr.addChild(id);

  family.addChild(items);
  clas.addChild(family);
  perishable.addChild(clas);
  item_nbr.addChild(perishable);
  date.addChild(item_nbr);

  type.addChild(holidays_events);
  locale.addChild(type);
  locale_name.addChild(locale);
  description.addChild(locale_name);
  transferred.addChild(description);
  date.addChild(transferred);
  store_nbr.addChild(date);

  t.addChild(store_nbr);

  return t;
}

void printVarOrder(const ExtendedVariableOrder& root) {
  std::cout << root.getName() << ' ' << root.isLeaf() << '\n';
  for (const ExtendedVariableOrder& x : root.getChildren()) {
    printVarOrder(x);
  }
}

void dropAll(pqxx::work& w, const ExtendedVariableOrder& root) {
  w.exec("DROP TABLE IF EXISTS " + root.getName() + "_type CASCADE;");
  w.exec("DROP VIEW IF EXISTS Q" + root.getName() + " CASCADE;");

  if (root.isLeaf()) {
    // DROP tables created by feature scaling
    w.exec("DROP TABLE IF EXISTS " + root.getName() + "_conv_type CASCADE;");
    w.exec("DROP TABLE IF EXISTS " + root.getName() + "_conv CASCADE;");
    w.exec("DROP VIEW IF EXISTS Q" + root.getName() + "_conv CASCADE;");

  } else {
    for (const auto& x : root.getChildren()) {
      dropAll(w, x);
    }
  }
}

void dropAll(pqxx::connection& c, const ExtendedVariableOrder& root) {
  pqxx::work w{c};
  for (const auto& x : root.getChildren()) {
    dropAll(w, x);
  }
  w.commit();
}

std::string stringOfVector(const std::vector<double>& array) {
  std::string out{"[ "};

  for (const double elem : array) {
    out += std::to_string(elem) + " | ";
  }

  // remove trailing " | "
  if (out.size() > 2) {
    out.erase(out.size() - 3, 3);
  }

  out += "]";
  return out;
}

double testResult(const std::vector<double>& theta, const std::vector<int>& x) {
  assert(x.size() + 2 == theta.size());
  double res{0.};
  for (size_t i{0}; i < x.size(); ++i) {
    res += theta.at(i + 1) * x.at(i);
  }
  return res + theta.back();
}

void testSales() {
  ExtendedVariableOrder varOrder{createSales()};
  printVarOrder(varOrder);
  std::cout << '\n';

  try {
    pqxx::connection c{"dbname=SalesWithNumbers hostaddr=127.0.0.1 port=5433"};
    if (c.is_open()) {
      std::cout << "Connected to: " << c.dbname() << '\n';

      dropAll(c, varOrder);
      std::cout << "Dropped all tables and views.\n";

      std::vector<std::string> relevantColumns{"Inventory", "Competitor", "Sale", "T"};
      double avg;

      std::vector<double> theta{linearRegression(varOrder, relevantColumns, c, avg)};
      std::cout << stringOfVector(theta) << '\n';

      assert(theta.size() == relevantColumns.size());

      c.disconnect();
    } else {
      std::cout << "Failed to connect!\n";
    }

  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
  }
}

std::vector<double> createRandomSales(pqxx::connection& c) {
  pqxx::work w{c};
  // DROP
  w.exec("DROP TABLE IF EXISTS Sales CASCADE;\
   DROP TABLE IF EXISTS Branch CASCADE;\
   DROP TABLE IF EXISTS Competition CASCADE;");

  // and recreate
  w.exec(
      "CREATE TABLE Competition(Location integer NOT NULL, Competitor integer NOT NULL) TABLESPACE thesis;");
  w.exec("CREATE TABLE Branch(Location integer NOT NULL, Product varchar(30) NOT NULL, Inventory integer NOT "
         "NULL) TABLESPACE thesis;");
  w.exec("CREATE TABLE Sales(Product varchar(30) NOT NULL, Sale integer NOT NULL) TABLESPACE thesis;");

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(3, 100);
  std::uniform_int_distribution<> valuesDis(-200, 200);

  // Competition Values
  std::string query = "INSERT INTO Competition VALUES";

  size_t numberCompetitors{static_cast<size_t>(dis(gen))};
  std::vector<int> competitor;
  competitor.reserve(numberCompetitors);

  for (size_t i{0}; i < numberCompetitors; ++i) {
    int x{valuesDis(gen)};
    competitor.push_back(x);

    query += "(" + std::to_string(i) + "," + std::to_string(x) + ")";

    if (i < numberCompetitors - 1) {
      query += ", ";
    } else {
      query += ";";
    }
  }

  w.exec(query);
  // END Competition

  // Sales VALUES
  query = "INSERT INTO Sales VALUES";

  size_t numberSales{static_cast<size_t>(dis(gen))};
  std::vector<int> sale;
  sale.reserve(numberSales);

  for (size_t i{0}; i < numberSales; ++i) {
    int x{valuesDis(gen)};
    sale.push_back(x);

    query += "('randomProduct" + std::to_string(i) + "'," + std::to_string(x) + ")";

    if (i < numberSales - 1) {
      query += ", ";
    } else {
      query += ";";
    }
  }

  w.exec(query);
  // END Sales

  // generate random factors
  std::uniform_real_distribution<> realDis(-20000., 20000.);
  std::vector<double> theta{realDis(gen), realDis(gen), realDis(gen)};

  // Branch VALUES
  query = "INSERT INTO Branch VALUES";

  for (size_t i{0}; i < numberCompetitors; ++i) {
    for (size_t j{0}; j < numberSales; ++j) {
      double res{theta.front() * competitor.at(i) + theta.at(1) * sale.at(j) + theta.back()};
      query +=
          "(" + std::to_string(i) + ",'randomProduct" + std::to_string(j) + "'," + std::to_string(res) + ")";

      if (i < numberCompetitors - 1 || j < numberSales - 1) {
        query += ", ";
      } else {
        query += ";";
      }
    }
  }

  w.exec(query);
  // END Branch

  w.commit();

  return theta;
}

void testRandom() {
  try {
    pqxx::connection c{"dbname=SalesWithNumbers hostaddr=127.0.0.1 port=5433"};
    if (c.is_open()) {
      std::cout << "Connected to: " << c.dbname() << '\n';

      bool stop{false};

      double maxRelError    = 0.;
      double minRelError    = INFINITY;
      double totalRelError  = 0.;
      double maxAbsError    = 0.;
      double minAbsError    = INFINITY;
      double totalAbsError  = 0.;
      double maxConsError   = 0.;
      double minConsError   = INFINITY;
      double totalConsError = 0.;

      size_t testCase{0};
      for (; testCase < 1000 && !stop; ++testCase) {
        std::vector<double> realTheta{createRandomSales(c)};
        // std::cout << stringOfVector(realTheta) << '\n';

        double avg{INFINITY};
        ExtendedVariableOrder varOrder{createSales()};
        // printVarOrder(varOrder);
        // std::cout << '\n';

        dropAll(c, varOrder);
        // std::cout << "Dropped all tables and views.\n";

        std::vector<std::string> relevantColumns{"Inventory", "Competitor", "Sale", "T"};

        std::vector<double> theta = linearRegression(varOrder, relevantColumns, c, avg);
        // std::cout << stringOfVector(theta) << '\n';

        assert(theta.size() == realTheta.size() + 1);
        // double sum{0.};
        for (size_t i{0}; i < realTheta.size(); ++i) {
          // sum += std::fabs(realTheta.at(i));

          // special case for constant term to check its relevance
          if (i == realTheta.size() - 1) {
            double error{std::fabs(theta.at(i + 1) - realTheta.at(i))};

            // update statistics
            if (maxConsError < error) {
              maxConsError = error;
            }
            if (minConsError > error) {
              minConsError = error;
            }
            totalConsError += error;

            if (std::fabs(error / realTheta.back()) > 0.05 && std::fabs(error / avg) > 0.01) {
              std::cout << "Got: " << theta.at(i + 1) << " but expected: " << realTheta.at(i) << '\n';
              std::cout << "Relevance error: " << error << '\n';
              std::cout << "Avg: " << avg << '\n';
              std::cout << "Testcase nr: " << testCase << '\n';
              std::cout << stringOfVector(theta) << '\n';
              std::cout << stringOfVector(realTheta) << '\n';
              stop = true;
            }

            // absolute error
          } else if (std::fabs(realTheta.at(i)) < 1.) {
            double error{std::fabs(theta.at(i + 1) - realTheta.at(i))};

            // update statistics
            if (maxAbsError < error) {
              maxAbsError = error;
            }
            if (minAbsError > error) {
              minAbsError = error;
            }
            totalAbsError += error;

            if (error > 0.5) {
              std::cout << "Got: " << theta.at(i + 1) << " but expected: " << realTheta.at(i) << '\n';
              std::cout << "Absolute error: " << error << '\n';
              std::cout << "Testcase nr: " << testCase << '\n';
              std::cout << stringOfVector(theta) << '\n';
              std::cout << stringOfVector(realTheta) << '\n';
              stop = true;
            }

            // relative error
          } else {
            double error{std::fabs((theta.at(i + 1) - realTheta.at(i)) / realTheta.at(i))};

            // update statistics
            if (maxRelError < error) {
              maxRelError = error;
            }
            if (minRelError > error) {
              minRelError = error;
            }
            totalRelError += error;

            if (error > 0.05) {
              std::cout << stringOfVector(theta) << '\n';
              std::cout << stringOfVector(realTheta) << '\n';
              std::cout << "Got: " << theta.at(i + 1) << " but expected: " << realTheta.at(i) << '\n';
              std::cout << "Relative error: " << error << '\n';
              std::cout << "Testcase nr: " << testCase << '\n';
              stop = true;
            }
          }
        }
      }

      c.disconnect();

      // test failed
      if (!stop) {
        std::cout << "All tests passed!\n";
      }

      std::cout << '\n';
      std::cout << "maxRelError: " << maxRelError << ", ";
      std::cout << "minRelError: " << minRelError << ", ";
      std::cout << "avgRelError: " << totalRelError / testCase << ", ";
      std::cout << '\n';
      std::cout << "maxAbsError: " << maxAbsError << ", ";
      std::cout << "minAbsError: " << minAbsError << ", ";
      std::cout << "avgAbsError: " << totalAbsError / testCase << ", ";
      std::cout << '\n';
      std::cout << "maxConsError: " << maxConsError << ", ";
      std::cout << "minConsError: " << minConsError << ", ";
      std::cout << "avgConsError: " << totalConsError / testCase << "\n";

    } else {
      std::cout << "Failed to connect!\n";
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    exit(1);
  }
}

void testFavorita() {
  ExtendedVariableOrder varOrder{createFavorita()};
  // printVarOrder(varOrder);
  // std::cout << '\n';

  try {
    pqxx::connection c{"dbname=Favorita hostaddr=127.0.0.1 port=5433"};
    if (c.is_open()) {
      std::cout << "Connected to: " << c.dbname() << '\n';

      dropAll(c, varOrder);
      std::cout << "Dropped all tables and views.\n";

      std::vector<std::string> relevantColumns{"unit_sales", "id",          "date", "store_nbr",
                                               "item_nbr",   "onpromotion", "T"};
      double avg;

      std::vector<double> theta{linearRegression(varOrder, relevantColumns, c, avg)};
      std::cout << "Linear regression complete.\n";
      std::cout << stringOfVector(theta) << '\n';
      assert(theta.size() == relevantColumns.size());

      c.disconnect();
    } else {
      std::cout << "Failed to connect!\n";
    }

  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
  }
}

int main() {
  // std::cout << std::boolalpha;

  // testSales();

  // std::cout << "\n\n";
  // testFavorita();

  testRandom();

  return 0;
}