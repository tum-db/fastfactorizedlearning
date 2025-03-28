#ifndef REGRESSION_H
#define REGRESSION_H

#include <pqxx/pqxx>
#include <vector>
#include <string>
#include "variableOrder.h"

// typedef std::string sql;

std::vector<scaleFactors> scaleFeatures(const std::vector<std::string>& relevantColumns,
                                        std::vector<ExtendedVariableOrder*>& leaves, const std::string& con);

void factorizeSQL(const ExtendedVariableOrder& varOrder, const std::string& con);
// void factorizeSQL(const ExtendedVariableOrder& varOrder, pqxx::work& transaction);
// sql factorizeSQL(const ExtendedVariableOrder& varOrder);

std::vector<double> batchGradientDescent(const std::vector<std::string>& relevantColumns,
                                         pqxx::connection& c);

std::vector<double> linearRegression(ExtendedVariableOrder& varOrder,
                                     const std::vector<std::string>& relevantColumns, const std::string& con,
                                     double& avg);

std::vector<double> naiveRegression(ExtendedVariableOrder& varOrder,
                                    const std::vector<std::string>& relevantColumns, const std::string& con,
                                    double& avg);

#endif
