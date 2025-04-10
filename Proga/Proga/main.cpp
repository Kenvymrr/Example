#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <algorithm>
#include "function.h"

using namespace std;

map<string, double> variables;
map<string, Function> functions;

string trim(const string& str);
bool isNumber(const string& str);
bool isValidIdentifier(const string& str);
double evaluateExpression(const string& expression, const map<string, double>& localVariables);
double executeFunction(const string& functionName, const vector<double>& arguments);
void defineVariable(const string& name, double value);
vector<string> tokenize(const string& expression);

void processLine(const string& line) {
    string trimmedLine = trim(line);
    if (trimmedLine.empty()) return;

    size_t funcDefPos = trimmedLine.find('(');
    size_t assignPos = trimmedLine.find('=');
    size_t printPos = trimmedLine.find("print");

    if (funcDefPos != string::npos && trimmedLine.find(')') != string::npos && trimmedLine.find(':') != string::npos) {
        string functionName = trimmedLine.substr(0, funcDefPos);
        string argumentsStr = trimmedLine.substr(funcDefPos + 1, trimmedLine.find(')') - funcDefPos - 1);
        string expression = trimmedLine.substr(trimmedLine.find(':') + 1);

        Function func;
        func.name = trim(functionName);
        func.expression = trim(expression);

        stringstream ss(argumentsStr);
        string arg;
        while (getline(ss, arg, ',')) {
            func.arguments.push_back(trim(arg));
        }

        functions[func.name] = func;
    }
    else if (assignPos != string::npos) {
        string varName = trimmedLine.substr(0, assignPos);
        string expression = trimmedLine.substr(assignPos + 1);

        varName = trim(varName);
        expression = trim(expression);

        if (varName.find('(') != string::npos && varName.find(')') != string::npos) {
            size_t start = varName.find('(');
            string newVarName = varName.substr(0, start);
            defineVariable(newVarName, evaluateExpression(expression));

        }
        else {
            if (!isValidIdentifier(varName)) {
                cerr << "Error: Invalid variable name: " << varName << endl;
                return;
            }

            variables[varName] = evaluateExpression(expression);
        }

    }
    else if (printPos != string::npos) {
        
        string printArgs = trimmedLine.substr(printPos + 5);
        printArgs = trim(printArgs);

        if (printArgs.empty()) {
            cout << "Variables:" << endl;
            for (const auto& pair : variables) {
                cout << pair.first << " = " << pair.second << endl;
            }
        }
        else {
            string varName = trim(printArgs);

            if (variables.count(varName)) {
                cout << varName << " = " << variables[varName] << endl;
            }
            else {
                cerr << "Error: Undefined variable: " << varName << endl;
            }
        }
    }
    else {
        cerr << "Error: Invalid instruction: " << trimmedLine << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: interpreter <filename>" << endl;
        return 1;
    }

    string filename = argv[1];
    ifstream inputFile(filename);

    if (!inputFile.is_open()) {
        cerr << "Error: Could not open file: " << filename << endl;
        return 1;
    }

    string line;
    while (getline(inputFile, line)) {
        processLine(line);
    }

    inputFile.close();

    variables.clear();
    functions.clear();

    return 0;
}