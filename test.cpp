#include <pqxx/pqxx>
#include <vector>
#include <string>
#include <iostream>
#include <cassert>
#include <cmath>
#include <random>
#include "variableOrder.h"
#include "regression.h"

const std::string g_favorita{"dbname=Favorita hostaddr=127.0.0.1 port=5433"};
const std::string g_sales{"dbname=SalesWithNumbers hostaddr=127.0.0.1 port=5433"};

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

ExtendedVariableOrder createTrain(const std::vector<std::string>& trainOrder, const size_t index = 0) {
  if (index == trainOrder.size()) {
    return {"train_conv", trainOrder};
  }

  ExtendedVariableOrder cur{trainOrder.at(index),
                            {trainOrder.begin(), trainOrder.begin() + static_cast<int>(index)}};
  cur.addChild(createTrain(trainOrder, index + 1));

  if (index == 0) {
    ExtendedVariableOrder t{"t"};
    t.addChild(cur);

    return t;
  }
  return cur;
}

ExtendedVariableOrder createFavorita(const std::vector<std::string>& trainOrder, const size_t index = 0) {
  if (index == trainOrder.size()) {
    return {"train_conv", trainOrder};
  }

  ExtendedVariableOrder cur{trainOrder.at(index),
                            {trainOrder.begin(), trainOrder.begin() + static_cast<int>(index)}};
  if (cur.getName() == "item_nbr") {
    ExtendedVariableOrder items{"items", {"item_nbr", "family", "class", "perishable"}};
    ExtendedVariableOrder clas{"class", {"item_nbr", "family", "perishable"}};
    ExtendedVariableOrder family{"family", {"item_nbr", "perishable"}, true};
    ExtendedVariableOrder perishable{"perishable", {"item_nbr"}};

    clas.addChild(items);
    family.addChild(clas);
    perishable.addChild(family);
    cur.addChild(perishable);

  } else if (cur.getName() == "date") {
    ExtendedVariableOrder transactions{"transactions_conv", {"date", "store_nbr", "txns"}};
    ExtendedVariableOrder txns{"txns", {"date", "store_nbr"}};

    txns.addChild(transactions);
    cur.addChild(txns);

    ExtendedVariableOrder oil{"oil_conv", {"date", "dcoilwtico"}};
    ExtendedVariableOrder dcoilwtico{"dcoilwtico", {"date"}};

    dcoilwtico.addChild(oil);
    cur.addChild(dcoilwtico);

    ExtendedVariableOrder holidays_events{
        "holidays_events_conv", {"date", "type", "locale", "locale_name", "description", "transferred"}};
    ExtendedVariableOrder description{
        "description", {"date", "type", "locale", "locale_name", "transferred"}, true};
    ExtendedVariableOrder locale_name{"locale_name", {"date", "type", "locale", "transferred"}, true};
    ExtendedVariableOrder type{"type", {"date", "locale", "transferred"}, true};
    ExtendedVariableOrder locale{"locale", {"date", "transferred"}, true};
    ExtendedVariableOrder transferred{"transferred", {"date"}, true};

    description.addChild(holidays_events);
    locale_name.addChild(description);
    type.addChild(locale_name);
    locale.addChild(type);
    transferred.addChild(locale);
    cur.addChild(transferred);

  } else if (cur.getName() == "store_nbr") {
    ExtendedVariableOrder stores{"stores", {"store_nbr", "city", "state", "stype", "cluster"}};
    ExtendedVariableOrder city{"city", {"store_nbr", "state", "stype", "cluster"}, true};
    ExtendedVariableOrder state{"state", {"store_nbr", "stype", "cluster"}, true};
    ExtendedVariableOrder cluster{"cluster", {"stype", "store_nbr"}};
    ExtendedVariableOrder stype{"stype", {"store_nbr"}, true};

    city.addChild(stores);
    state.addChild(city);
    cluster.addChild(state);
    stype.addChild(cluster);
    cur.addChild(stype);
  }

  cur.addChild(createFavorita(trainOrder, index + 1));

  if (index == 0) {
    ExtendedVariableOrder t{"t"};
    t.addChild(cur);

    return t;
  }
  return cur;
}

ExtendedVariableOrder createFavorita() {
  ExtendedVariableOrder t{"t"};

  ExtendedVariableOrder items{"items", {"item_nbr", "family", "class", "perishable"}};
  ExtendedVariableOrder clas{"class", {"item_nbr", "family", "perishable"}};
  ExtendedVariableOrder family{"family", {"item_nbr", "perishable"}, true};
  ExtendedVariableOrder perishable{"perishable", {"item_nbr"}};
  ExtendedVariableOrder item_nbr{"item_nbr", {"date", "store_nbr", "onpromotion"}};

  ExtendedVariableOrder holidays_events{
      "holidays_events_conv", {"date", "type", "locale", "locale_name", "description", "transferred"}};
  ExtendedVariableOrder description{
      "description", {"date", "type", "locale", "locale_name", "transferred"}, true};
  ExtendedVariableOrder locale_name{"locale_name", {"date", "type", "locale", "transferred"}, true};
  ExtendedVariableOrder type{"type", {"date", "locale", "transferred"}, true};
  ExtendedVariableOrder locale{"locale", {"date", "transferred"}, true};
  ExtendedVariableOrder transferred{"transferred", {"date"}, true};
  ExtendedVariableOrder date{"date", {"store_nbr", "onpromotion"}};

  ExtendedVariableOrder oil{"oil_conv", {"date", "dcoilwtico"}};
  ExtendedVariableOrder dcoilwtico{"dcoilwtico", {"date"}};

  ExtendedVariableOrder transactions{"transactions_conv", {"date", "store_nbr", "txns"}};
  ExtendedVariableOrder txns{"txns", {"date", "store_nbr"}};
  ExtendedVariableOrder store_nbr{"store_nbr", {"onpromotion"}};

  ExtendedVariableOrder stores{"stores", {"store_nbr", "city", "state", "stype", "cluster"}};
  ExtendedVariableOrder city{"city", {"store_nbr", "state", "stype", "cluster"}, true};
  ExtendedVariableOrder state{"state", {"store_nbr", "stype", "cluster"}, true};
  ExtendedVariableOrder cluster{"cluster", {"stype", "store_nbr"}};
  ExtendedVariableOrder stype{"stype", {"store_nbr"}, true};

  ExtendedVariableOrder train{"train_conv_small",
                              {"id", "date", "store_nbr", "item_nbr", "unit_sales", "onpromotion"}};
  ExtendedVariableOrder id{"id", {"date", "store_nbr", "item_nbr", "unit_sales", "onpromotion"}};
  ExtendedVariableOrder unit_sales{"unit_sales", {"date", "store_nbr", "item_nbr", "onpromotion"}};
  ExtendedVariableOrder onpromotion{"onpromotion"};

  city.addChild(stores);
  state.addChild(city);
  cluster.addChild(state);
  stype.addChild(cluster);
  store_nbr.addChild(stype);

  txns.addChild(transactions);
  date.addChild(txns);

  dcoilwtico.addChild(oil);
  date.addChild(dcoilwtico);

  id.addChild(train);
  unit_sales.addChild(id);
  // onpromotion.addChild(unit_sales);
  item_nbr.addChild(unit_sales);

  clas.addChild(items);
  family.addChild(clas);
  perishable.addChild(family);
  item_nbr.addChild(perishable);
  date.addChild(item_nbr);

  description.addChild(holidays_events);
  locale_name.addChild(description);
  type.addChild(locale_name);
  locale.addChild(type);
  transferred.addChild(locale);
  date.addChild(transferred);
  store_nbr.addChild(date);

  onpromotion.addChild(store_nbr);

  t.addChild(onpromotion);

  return t;
}

void printVarOrder(const ExtendedVariableOrder& root) {
  std::cout << root.getName() << ' ' << root.isLeaf() << std::endl;
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
    w.exec("DROP VIEW IF EXISTS " + root.getName() + "_conv CASCADE;");
    w.exec("DROP VIEW IF EXISTS Q" + root.getName() + "_conv CASCADE;");

  } else {
    for (const auto& x : root.getChildren()) {
      dropAll(w, x);
    }
  }
}

void dropAll(const std::string& con, const ExtendedVariableOrder& root) {
  pqxx::connection c{con};
  pqxx::work w{c};
  for (const auto& x : root.getChildren()) {
    dropAll(w, x);
  }
  w.exec("DROP TABLE IF EXISTS Q" + root.getName() + " CASCADE;");
  w.commit();
  c.disconnect();
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
  // printVarOrder(varOrder);
  // std::cout << std::endl;

  try {
    dropAll(g_sales, varOrder);
    std::cout << "Dropped all tables and views." << std::endl;

    std::vector<std::string> relevantColumns{"Inventory", "Competitor", "Sale", "T"};
    double avg;

    std::vector<double> theta{linearRegression(varOrder, relevantColumns, g_sales, avg)};
    // std::vector<double> theta{naiveRegression(varOrder, relevantColumns, g_sales, avg)};
    std::cout << stringOfVector(theta) << std::endl;

    assert(theta.size() == relevantColumns.size());

  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}

std::vector<double> createRandomSales(pqxx::connection& c) {
  pqxx::work w{c};
  // DROP
  w.exec("DROP TABLE IF EXISTS Sales CASCADE;\
   DROP TABLE IF EXISTS Branch CASCADE;\
   DROP TABLE IF EXISTS Competition CASCADE;");

  // and recreate
  w.exec("CREATE TABLE Competition(Location integer NOT NULL, Competitor integer NOT NULL);");
  w.exec("CREATE TABLE Branch(Location integer NOT NULL, Product varchar(30) NOT NULL, Inventory integer NOT "
         "NULL);");
  w.exec("CREATE TABLE Sales(Product varchar(30) NOT NULL, Sale integer NOT NULL);");

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
    pqxx::connection c{g_sales};
    if (c.is_open()) {
      std::cout << "Connected to: " << c.dbname() << std::endl;

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
        // std::cout << stringOfVector(realTheta) << std::endl;

        double avg{INFINITY};
        ExtendedVariableOrder varOrder{createSales()};
        // printVarOrder(varOrder);
        // std::cout << std::endl;

        dropAll(g_sales, varOrder);
        // std::cout << "Dropped all tables and views."<<std::endl;

        std::vector<std::string> relevantColumns{"Inventory", "Competitor", "Sale", "T"};

        std::vector<double> theta = linearRegression(varOrder, relevantColumns, g_sales, avg);
        // std::vector<double> theta = naiveRegression(varOrder, relevantColumns, g_sales, avg);
        // std::cout << stringOfVector(theta) << std::endl;

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
              std::cout << "Got: " << theta.at(i + 1) << " but expected: " << realTheta.at(i) << std::endl;
              std::cout << "Relevance error: " << error << std::endl;
              std::cout << "Avg: " << avg << std::endl;
              std::cout << "Testcase nr: " << testCase << std::endl;
              std::cout << stringOfVector(theta) << std::endl;
              std::cout << stringOfVector(realTheta) << std::endl;
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
              std::cout << "Got: " << theta.at(i + 1) << " but expected: " << realTheta.at(i) << std::endl;
              std::cout << "Absolute error: " << error << std::endl;
              std::cout << "Testcase nr: " << testCase << std::endl;
              std::cout << stringOfVector(theta) << std::endl;
              std::cout << stringOfVector(realTheta) << std::endl;
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
              std::cout << stringOfVector(theta) << std::endl;
              std::cout << stringOfVector(realTheta) << std::endl;
              std::cout << "Got: " << theta.at(i + 1) << " but expected: " << realTheta.at(i) << std::endl;
              std::cout << "Relative error: " << error << std::endl;
              std::cout << "Testcase nr: " << testCase << std::endl;
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

      std::cout << std::endl;
      std::cout << "maxRelError: " << maxRelError << ", ";
      std::cout << "minRelError: " << minRelError << ", ";
      std::cout << "avgRelError: " << totalRelError / testCase << ", ";
      std::cout << std::endl;
      std::cout << "maxAbsError: " << maxAbsError << ", ";
      std::cout << "minAbsError: " << minAbsError << ", ";
      std::cout << "avgAbsError: " << totalAbsError / testCase << ", ";
      std::cout << std::endl;
      std::cout << "maxConsError: " << maxConsError << ", ";
      std::cout << "minConsError: " << minConsError << ", ";
      std::cout << "avgConsError: " << totalConsError / testCase << "\n";

    } else {
      std::cout << "Failed to connect!\n";
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    exit(1);
  }
}

std::string stringOfVector(const std::vector<std::string>& array) {
  std::string out{"[ "};

  for (const auto& elem : array) {
    out += elem + " | ";
  }

  // remove trailing " | "
  if (out.size() > 2) {
    out.erase(out.size() - 3, 3);
  }

  out += " ]";
  return out;
}

void compareVarOrder(const ExtendedVariableOrder& root1, const ExtendedVariableOrder& root2) {
  std::cout << root1.getName() << " = " << root2.getName() << std::endl;
  std::cout << root1.isLeaf() << " = " << root2.isLeaf() << std::endl;
  assert(root1.getName() == root2.getName());
  assert(root1.isLeaf() == root2.isLeaf());

  std::cout << stringOfVector(root1.getKey()) << std::endl;
  std::cout << stringOfVector(root2.getKey()) << std::endl;

  std::cout << std::endl;

  for (const ExtendedVariableOrder& x : root1.getChildren()) {
    bool found = false;
    for (const ExtendedVariableOrder& y : root2.getChildren()) {
      if (x.getName() != y.getName()) {
        continue;
      }
      compareVarOrder(x, y);
      found = true;
    }

    if (!found) {
      std::cout << root2.getName() << " is missing " << x.getName() << std::endl;
      assert(false);
    }
  }

  if (root1.getChildren().size() != root2.getChildren().size()) {
    std::cout << root1.getName() << " is missing something \n";
    assert(false);
  }
}

void testFavorita() {
  // ExtendedVariableOrder varOrder1{createFavorita()};
  std::vector<std::string> trainOrder{"onpromotion", "item_nbr", "unit_sales", "store_nbr", "date"};
  ExtendedVariableOrder varOrder{createFavorita(trainOrder)};
  // ExtendedVariableOrder varOrder{createTrain(trainOrder)};
  // printVarOrder(varOrder);
  // std::cout << std::endl;

  // compareVarOrder(varOrder1, varOrder);
  // std::cout << std::endl;
  // assert(false);

  try {
    dropAll(g_favorita, varOrder);
    std::cout << "Dropped all tables and views." << std::endl;

    std::vector<std::string> relevantColumns{"unit_sales", "date",        "store_nbr",
                                             "item_nbr",   "onpromotion", "T"};
    double avg;

    std::vector<double> theta{linearRegression(varOrder, relevantColumns, g_favorita, avg)};
    // std::vector<double> theta{naiveRegression(varOrder, relevantColumns, g_favorita, avg)};
    std::cout << "Linear regression complete.\n";
    std::cout << stringOfVector(theta) << std::endl;
    assert(theta.size() == relevantColumns.size());

  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}

void testMadlib() {
  try {
    pqxx::connection c{g_favorita};
    if (c.is_open()) {
      std::cout << "Connected to: " << c.dbname() << std::endl;

      pqxx::work transaction{c};

      transaction.exec("DROP VIEW IF EXISTS madlibView;");
      transaction.exec("DROP TABLE IF EXISTS madlib_linregr;");

      transaction.exec("CREATE VIEW madlibView AS ("
                       "SELECT*"
                       "FROM train_conv NATURAL JOIN items NATURAL JOIN holidays_events_conv NATURAL JOIN "
                       "oil_conv NATURAL JOIN stores NATURAL JOIN transactions_conv);");

      transaction.exec("SELECT linregr_train( 'madlibView',"
                       "'madlib_linregr',"
                       "'unit_sales',"
                       " 'ARRAY[1, date, store_nbr, item_nbr, onpromotion]' "
                       ");");

      transaction.commit();

      c.disconnect();
    } else {
      std::cout << "Failed to connect!\n";
    }

  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}

void computeJoin() {
  try {
    pqxx::connection c{g_favorita};
    if (c.is_open()) {
      std::cout << "Connected to: " << c.dbname() << std::endl;

      pqxx::nontransaction transaction{c};

      transaction.exec("CREATE TABLE complete_join AS ("
                       "SELECT *"
                       "FROM train_conv NATURAL JOIN items NATURAL JOIN holidays_events_conv NATURAL JOIN "
                       "oil_conv NATURAL JOIN stores NATURAL JOIN transactions_conv);");

      transaction.exec("CREATE TABLE item_date_join AS ("
                       "SELECT *"
                       "FROM train_conv NATURAL JOIN items NATURAL JOIN holidays_events_conv NATURAL JOIN "
                       "oil_conv NATURAL JOIN transactions_conv);");

      transaction.exec(
          "CREATE TABLE item_store_join AS ("
          "SELECT *"
          "FROM train_conv NATURAL JOIN items NATURAL JOIN stores NATURAL JOIN transactions_conv);");

      transaction.exec("CREATE TABLE date_store_join AS ("
                       "SELECT *"
                       "FROM train_conv NATURAL JOIN holidays_events_conv NATURAL JOIN "
                       "oil_conv NATURAL JOIN stores NATURAL JOIN transactions_conv);");

      transaction.exec("CREATE TABLE item_join AS ("
                       "SELECT *"
                       "FROM train_conv NATURAL JOIN items);");

      transaction.exec("CREATE TABLE date_join AS ("
                       "SELECT *"
                       "FROM train_conv NATURAL JOIN holidays_events_conv NATURAL JOIN "
                       "oil_conv NATURAL JOIN transactions_conv);");

      transaction.exec("CREATE TABLE store_join AS ("
                       "SELECT *"
                       "FROM train_conv NATURAL JOIN stores NATURAL JOIN transactions_conv);");

      transaction.commit();

      c.disconnect();
    } else {
      std::cout << "Failed to connect!\n";
    }

  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}

int main() {
  // std::cout << std::boolalpha;

  // computeJoin();
  // testMadlib();
  // testSales();

  // std::cout << "\n\n";
  testFavorita();

  // testRandom();

  return 0;
}