#ifndef FUNCTION_H
#define FUNCTION_H

#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace std;

struct Function {
    string name;
    vector<string> arguments;
    string expression;
};

double evaluateExpression(const string& expression, const map<string, double>& localVariables = {});
double executeFunction(const string& functionName, const vector<double>& arguments);
void defineVariable(const string& name, double value);
vector<string> tokenize(const string& expression);

#endif