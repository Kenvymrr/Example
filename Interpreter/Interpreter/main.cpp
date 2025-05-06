#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <stack>
#include <memory>
#include <stdexcept>
#include <cctype>
#include <cmath>

using namespace std;

enum class VarType { INTEGER, FLOAT };

struct Variable {
    VarType type;
    union {
        int iValue;
        double fValue;
    };
    Variable(int val) : type(VarType::INTEGER), iValue(val) {}
    Variable(double val) : type(VarType::FLOAT), fValue(val) {}
    double getValue() const { return type == VarType::INTEGER ? iValue : fValue; }
    ~Variable() {}
};

struct Function {
    vector<string> params;
    string expression;
};

class Interpreter {
private:
    map<string, unique_ptr<Variable>> variables;
    map<string, unique_ptr<Function>> functions;

    // Удаление пробелов
    string trim(const string& str) {
        size_t first = str.find_first_not_of(" \t");
        if (first == string::npos) return "";
        size_t last = str.find_last_not_of(" \t");
        return str.substr(first, last - first + 1);
    }

    // Проверка валидности имени
    bool isValidName(const string& name) {
        if (name.empty() || isdigit(name[0])) return false;
        for (char c : name) {
            if (!isalnum(c)) return false;
        }
        return true;
    }

    // Разбор параметров функции
    vector<string> parseParameters(const string& paramStr) {
        vector<string> params;
        stringstream ss(paramStr);
        string param;
        while (getline(ss, param, ',')) {
            param = trim(param);
            if (!param.empty()) params.push_back(param);
        }
        return params;
    }

    // Разбор выражения
    string parseExpression(const string& expr) {
        string cleanExpr = trim(expr);
        if (cleanExpr.back() == ';') {
            cleanExpr.pop_back();
        }
        return cleanExpr;
    }

    // Приоритет операторов
    int getPrecedence(char op) {
        if (op == '+' || op == '-') return 1;
        if (op == '*' || op == '/') return 2;
        return 0;
    }

    // Выполнение арифметической операции
    double applyOperator(double a, double b, char op) {
        switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/':
            if (abs(b) < 1e-10) throw runtime_error("Division by zero");
            return a / b;
        default: throw runtime_error("Unknown operator: " + string(1, op));
        }
    }

    // Проверка, является ли строка числом
    bool isNumber(const string& str) {
        try {
            size_t pos;
            double value = std::stod(str, &pos);
            return pos == str.length();
        }
        catch (...) {
            return false;
        }
    }

    // Разбор и вычисление выражения
    double evaluateExpression(const string& expr, map<string, double> localVars = {}) {
        stack<double> values;
        stack<char> operators;
        string token;
        size_t i = 0;
        bool expectOperand = true;

        while (i < expr.length()) {
            char c = expr[i];
            if (isspace(c)) {
                i++;
                continue;
            }

            // Обработка скобок
            if (c == '(') {
                if (!expectOperand && token.empty()) {
                    operators.push('*'); // Неявное умножение
                    expectOperand = true;
                }
                int parenCount = 1;
                string subExpr;
                i++;
                while (i < expr.length() && parenCount > 0) {
                    if (expr[i] == '(') parenCount++;
                    else if (expr[i] == ')') parenCount--;
                    if (parenCount > 0) subExpr += expr[i];
                    i++;
                }
                if (parenCount != 0) throw runtime_error("Mismatched parentheses in expression: " + expr);

                if (!token.empty() && functions.find(token) != functions.end()) {
                    // Вызов функции
                    string funcName = token;
                    vector<double> args;
                    string argToken;
                    int innerParenCount = 0;
                    for (size_t j = 0; j <= subExpr.length(); ++j) {
                        char ch = (j < subExpr.length()) ? subExpr[j] : ',';
                        if (j == subExpr.length()) {
                            if (!argToken.empty()) {
                                args.push_back(evaluateExpression(trim(argToken), localVars));
                                argToken.clear();
                            }
                            break;
                        }
                        if (ch == '(') innerParenCount++;
                        else if (ch == ')') innerParenCount--;
                        else if (ch == ',' && innerParenCount == 0) {
                            if (!argToken.empty()) {
                                args.push_back(evaluateExpression(trim(argToken), localVars));
                                argToken.clear();
                            }
                            continue;
                        }
                        argToken += ch;
                    }

                    auto& func = functions[funcName];
                    if (args.size() != func->params.size()) {
                        throw runtime_error("Incorrect number of arguments for function " + funcName + ": expected " +
                            to_string(func->params.size()) + ", got " + to_string(args.size()) + " in expression: " + expr);
                    }
                    map<string, double> funcVars = localVars;
                    for (size_t j = 0; j < args.size(); ++j) {
                        funcVars[func->params[j]] = args[j];
                    }
                    values.push(evaluateExpression(func->expression, funcVars));
                    token.clear();
                    expectOperand = false;
                    continue;
                }
                else {
                    // Подвыражение в скобках
                    if (!token.empty()) {
                        if (isNumber(token)) {
                            values.push(stod(token));
                            expectOperand = false;
                        }
                        else if (variables.find(token) != variables.end()) {
                            values.push(variables[token]->getValue());
                            expectOperand = false;
                        }
                        else if (localVars.find(token) != localVars.end()) {
                            values.push(localVars[token]);
                            expectOperand = false;
                        }
                        else {
                            throw runtime_error("Undefined variable or function: " + token + " in expression: " + expr);
                        }
                        token.clear();
                    }
                    values.push(evaluateExpression(subExpr, localVars));
                    expectOperand = false;
                    continue;
                }
            }

            // Обработка операторов
            if (c == '+' || c == '-' || c == '*' || c == '/') {
                if (!token.empty()) {
                    if (isNumber(token)) {
                        values.push(stod(token));
                        expectOperand = false;
                    }
                    else if (variables.find(token) != variables.end()) {
                        values.push(variables[token]->getValue());
                        expectOperand = false;
                    }
                    else if (localVars.find(token) != localVars.end()) {
                        values.push(localVars[token]);
                        expectOperand = false;
                    }
                    else {
                        throw runtime_error("Undefined variable or function: " + token + " in expression: " + expr);
                    }
                    token.clear();
                }
                while (!operators.empty() && operators.top() != '(' &&
                    getPrecedence(operators.top()) >= getPrecedence(c)) {
                    char op = operators.top();
                    operators.pop();
                    if (values.size() < 2) throw runtime_error("Invalid expression: " + expr);
                    double b = values.top(); values.pop();
                    double a = values.top(); values.pop();
                    values.push(applyOperator(a, b, op));
                }
                operators.push(c);
                expectOperand = true;
                i++;
                continue;
            }

            // Накопление токена
            token += c;
            i++;
        }

        // Обработка последнего токена
        if (!token.empty()) {
            if (isNumber(token)) {
                values.push(stod(token));
                expectOperand = false;
            }
            else if (variables.find(token) != variables.end()) {
                values.push(variables[token]->getValue());
                expectOperand = false;
            }
            else if (localVars.find(token) != localVars.end()) {
                values.push(localVars[token]);
                expectOperand = false;
            }
            else {
                throw runtime_error("Undefined variable or function: " + token + " in expression: " + expr);
            }
        }

        // Вычисление оставшихся операторов
        while (!operators.empty()) {
            char op = operators.top();
            operators.pop();
            if (values.size() < 2) throw runtime_error("Invalid expression: " + expr);
            double b = values.top(); values.pop();
            double a = values.top(); values.pop();
            values.push(applyOperator(a, b, op));
        }

        if (values.empty()) throw runtime_error("Empty expression: " + expr);
        return values.top();
    }

public:
    ~Interpreter() {}

    void processInstruction(const string& line) {
        string trimmed = trim(line);
        if (trimmed.empty()) return;

        // Обработка определения функции
        if (trimmed.find(':') != string::npos) {
            size_t colonPos = trimmed.find(':');
            string nameParams = trim(trimmed.substr(0, colonPos));
            string expr = parseExpression(trimmed.substr(colonPos + 1));

            size_t openParen = nameParams.find('(');
            if (openParen != string::npos) {
                size_t closeParen = nameParams.find(')');
                string funcName = trim(nameParams.substr(0, openParen));
                string paramsStr = nameParams.substr(openParen + 1, closeParen - openParen - 1);

                if (!isValidName(funcName)) {
                    throw runtime_error("Invalid function name: " + funcName);
                }

                auto func = make_unique<Function>();
                func->params = parseParameters(paramsStr);
                func->expression = expr;
                functions[funcName] = move(func);
                return;
            }
        }

        // Обработка присваивания переменной
        if (trimmed.find('=') != string::npos) {
            size_t eqPos = trimmed.find('=');
            string left = trim(trimmed.substr(0, eqPos));
            string right = trim(trimmed.substr(eqPos + 1));

            // Определение переменной с типом
            size_t openParen = left.find('(');
            if (openParen != string::npos) {
                size_t closeParen = left.find(')');
                string varName = trim(left.substr(0, openParen));
                string typeStr = trim(left.substr(openParen + 1, closeParen - openParen - 1));

                if (!isValidName(varName)) {
                    throw runtime_error("Invalid variable name: " + varName);
                }

                if (right.back() == ';') {
                    right.pop_back();
                }

                if (typeStr == "i") {
                    double value = evaluateExpression(right);
                    if (abs(value - round(value)) > 1e-10) {
                        throw runtime_error("Non-integer value for integer variable: " + varName);
                    }
                    variables[varName] = make_unique<Variable>(static_cast<int>(value));
                }
                else if (typeStr == "f") {
                    double value = evaluateExpression(right);
                    variables[varName] = make_unique<Variable>(value);
                }
                else {
                    throw runtime_error("Unknown type for variable: " + typeStr);
                }
                return;
            }

            // Присваивание переменной
            if (right.back() == ';') {
                right.pop_back();
            }

            if (!isValidName(left)) {
                throw runtime_error("Invalid variable name: " + left);
            }

            double value = evaluateExpression(right);
            if (abs(value - round(value)) < 1e-10) {
                variables[left] = make_unique<Variable>(static_cast<int>(value));
            }
            else {
                variables[left] = make_unique<Variable>(value);
            }
            return;
        }

        // Обработка команды print
        if (trimmed.find("print") == 0) {
            if (trimmed == "print;" || trimmed == "print") {
                for (const auto& var : variables) {
                    cout << var.first << " = ";
                    if (var.second->type == VarType::INTEGER) {
                        cout << var.second->iValue;
                    }
                    else {
                        cout << var.second->fValue;
                    }
                    cout << endl;
                }
            }
            else {
                string varName = trim(trimmed.substr(5));
                if (varName.back() == ';') {
                    varName.pop_back();
                }

                auto it = variables.find(varName);
                if (it == variables.end()) {
                    throw runtime_error("Variable not found: " + varName);
                }

                cout << varName << " = ";
                if (it->second->type == VarType::INTEGER) {
                    cout << it->second->iValue;
                }
                else {
                    cout << it->second->fValue;
                }
                cout << endl;
            }
            return;
        }

        throw runtime_error("Unknown instruction: " + trimmed);
    }

    // Загрузка и обработка файла
    void run(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            throw runtime_error("Could not open file: " + filename);
        }

        string line;
        while (getline(file, line)) {
            try {
                processInstruction(line);
            }
            catch (const runtime_error& e) {
                cerr << "Error in line: \"" << line << "\": " << e.what() << endl;
                file.close();
                return;
            }
        }
        file.close();
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    try {
        Interpreter interpreter;
        interpreter.run(argv[1]);
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}