#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <cctype>
#include <cmath>
#include "function.h"

using namespace std;

extern map<string, double> variables;
extern map<string, Function> functions;

string trim(const string& str) {
    size_t first = str.find_first_not_of(' ');
    if (string::npos == first) {
        return "";
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

bool isNumber(const string& str) {
    if (str.empty()) return false;
    bool hasDecimal = false;
    for (char c : str) {
        if (isdigit(c)) continue;
        if (c == '.' && !hasDecimal) {
            hasDecimal = true;
            continue;
        }
        return false;
    }
    return true;
}

bool isValidIdentifier(const string& str) {
    if (str.empty() || !isalpha(str[0])) return false;
    for (char c : str) {
        if (!isalnum(c)) return false;
    }
    return true;
}

int getPrecedence(const string& op) {
    if (op == "+" || op == "-") return 1;
    if (op == "*" || op == "/") return 2;
    return 0;
}

void defineVariable(const string& name, double value) {
    variables[name] = value;
}

double executeFunction(const string& functionName, const vector<double>& arguments) {
    if (functions.count(functionName) == 0) {
        cerr << "Error: Undefined function: " << functionName << endl;
        return 0.0;
    }

    Function func = functions[functionName];

    if (func.arguments.size() != arguments.size()) {
        cerr << "Error: Incorrect number of arguments for function: " << functionName << endl;
        return 0.0;
    }

    map<string, double> localVariables;
    for (size_t i = 0; i < func.arguments.size(); ++i) {
        localVariables[func.arguments[i]] = arguments[i];
    }

    return evaluateExpression(func.expression, localVariables);
}

vector<string> tokenize(const string& expression) {
    vector<string> tokens;
    stringstream ss(expression);
    string token;

    while (ss >> token) {
        if (token == "+" || token == "-" || token == "*" || token == "/" || token == "(" || token == ")") {
            tokens.push_back(token);
        }
        else {
            size_t pos = 0;
            while (pos < token.length()) {
                size_t nextPos = token.find_first_of("+-*/()", pos);
                if (nextPos == string::npos) {
                    tokens.push_back(token.substr(pos));
                    pos = token.length();
                }
                else {
                    if (nextPos > pos) {
                        tokens.push_back(token.substr(pos, nextPos - pos));
                    }
                    tokens.push_back(string(1, token[nextPos]));
                    pos = nextPos + 1;
                }
            }
        }
    }
    return tokens;
}

double evaluateExpression(const string& expression, const map<string, double>& localVariables) {
    vector<string> tokens = tokenize(expression);

    stack<double> numbers;
    stack<string> operations;

    for (const string& token : tokens) {
        if (isNumber(token)) {
            numbers.push(stod(token));
        }
        else if (isValidIdentifier(token)) {
            double value = 0.0;
            if (localVariables.count(token)) {
                value = localVariables.at(token);
            }
            else if (variables.count(token)) {
                value = variables.at(token);
            }
            else if (functions.count(token)) {
                value = executeFunction(token, {});
            }
            else {
                cerr << "Error: Undefined variable or function: " << token << endl;
                return 0.0;
            }
            numbers.push(value);
        }
        else if (token == "(") {
            operations.push(token);
        }
        else if (token == ")") {
            while (!operations.empty() && operations.top() != "(") {
                string op = operations.top();
                operations.pop();
                double val2 = numbers.top();
                numbers.pop();
                double val1 = numbers.top();
                numbers.pop();

                if (op == "+") numbers.push(val1 + val2);
                else if (op == "-") numbers.push(val1 - val2);
                else if (op == "*") numbers.push(val1 * val2);
                else if (op == "/") numbers.push(val1 / val2);
            }
            if (!operations.empty()) operations.pop();
        }
        else if (token == "+" || token == "-" || token == "*" || token == "/") {
            while (!operations.empty() && operations.top() != "(" && getPrecedence(token) <= getPrecedence(operations.top())) {
                string op = operations.top();
                operations.pop();
                double val2 = numbers.top();
                numbers.pop();
                double val1 = numbers.top();
                numbers.pop();

                if (op == "+") numbers.push(val1 + val2);
                else if (op == "-") numbers.push(val1 - val2);
                else if (op == "*") numbers.push(val1 * val2);
                else if (op == "/") numbers.push(val1 / val2);
            }
            operations.push(token);
        }
        else {
            cerr << "Error: Invalid expression token: " << token << endl;
            return 0.0;
        }
    }

    while (!operations.empty()) {
        string op = operations.top();
        operations.pop();
        double val2 = numbers.top();
        numbers.pop();
        double val1 = numbers.top();
        numbers.pop();

        if (op == "+") numbers.push(val1 + val2);
        else if (op == "-") numbers.push(val1 - val2);
        else if (op == "*") numbers.push(val1 * val2);
        else if (op == "/") numbers.push(val1 / val2);
    }

    if (!numbers.empty()) {
        return numbers.top();
    }
    else {
        return 0.0;
    }
}