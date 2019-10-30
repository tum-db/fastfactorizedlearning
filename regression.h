#ifndef REGRESSION_H
#define REGRESSION_H

#include <pqxx/pqxx>
#include "variableOrder.h"

// typedef std::string sql;

void factorizeSQL(const ExtendedVariableOrder& varOrder, pqxx::connection& c);
// void factorizeSQL(const ExtendedVariableOrder& varOrder, pqxx::work& transaction);
// sql factorizeSQL(const ExtendedVariableOrder& varOrder);

std::vector<double> batchGradientDescent(const std::vector<std::string>& varOrder, pqxx::connection& c);

#endif
