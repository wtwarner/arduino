#ifndef TEMPLATETANGO_H
#define TEMPLATETANGO_H

#include <Arduino.h>
#include <map>
#include <regex>
#include <sstream>
#include <cmath>
#include <stack>

class TemplateTango {
public:
    /**
     * @brief Render the template string with the provided variables.
     * @param templateStr The template string containing placeholders for variables.
     * @param variables A map of variable names and their corresponding values.
     * @return A string with variables replaced by their corresponding values.
     */
    static String render(const String& templateStr, const std::map<String, String>& variables) {
        String result = templateStr;
        std::regex varRegex("\\{\\{(.*?)\\}\\}");
        std::smatch matches;
        std::string resultStd = result.c_str();  // Convert to std::string for regex

        while (std::regex_search(resultStd, matches, varRegex)) {
            std::string varExpression = matches[1].str();
            //Serial.printf("Found variable expression: %s\n", varExpression.c_str());
            // Evaluate the expression
            String evaluated = evaluateExpression(varExpression.c_str(), variables);
            resultStd.replace(matches.position(0), matches.length(0), evaluated.c_str());
        }
        result = resultStd.c_str();  // Convert back to String
        return result;
    }

private:
    /**
     * @brief Evaluate an expression by replacing variables and computing the result.
     * @param expression The expression to evaluate.
     * @param variables A map of variable names and their corresponding values.
     * @return The evaluated result as a string.
     */
    static String evaluateExpression(const String& expression, const std::map<String, String>& variables) {
        // Replace variables with their values
        std::string expr = expression.c_str();
        for (const auto& var : variables) {
            //Serial.printf("Replacing variable %s with value %s\n", var.first.c_str(), var.second.c_str());
            std::regex varRegex("\\b" + std::string(var.first.c_str()) + "\\b");
            expr = std::regex_replace(expr, varRegex, var.second.c_str());
        }

        // Handle string concatenation with **
        std::regex concatRegex(R"((\"[^\"]*\")\s*\*\*\s*(\"[^\"]*\"))");
        std::smatch concatMatch;
        while (std::regex_search(expr, concatMatch, concatRegex)) {
            std::string left = concatMatch[1].str();
            std::string right = concatMatch[2].str();
            left = left.substr(1, left.size() - 2);  // Remove quotes
            right = right.substr(1, right.size() - 2);  // Remove quotes
            std::string concatenated = "\"" + left + right + "\"";
            expr.replace(concatMatch.position(0), concatMatch.length(0), concatenated);
        }

        // Evaluate arithmetic expressions only if operators are found
        if (std::regex_search(expr, std::regex("[+\\-*/%()]"))) {
            double result = evaluateArithmetic(expr);
            // Remove trailing zeroes and decimal point if not needed
            std::string resultStr = std::to_string(result);
            resultStr.erase(resultStr.find_last_not_of('0') + 1, std::string::npos);
            if (resultStr.back() == '.') {
                resultStr.pop_back();
            }
            //Serial.printf("Evaluated expression: %s = %s\n", expression.c_str(), resultStr.c_str());
            return resultStr.c_str();
        } else {
            // Return the replaced expression if no arithmetic operations are found
            return expr.c_str();
        }
    }

    /**
     * @brief Evaluate a basic arithmetic expression using the Shunting Yard algorithm.
     * @param expr The arithmetic expression to evaluate.
     * @return The result of the expression as a double.
     */
    static double evaluateArithmetic(const std::string& expr) {
        std::istringstream input(expr);
        std::stack<double> values;
        std::stack<char> ops;
        while (input) {
            char token = input.peek();
            if (isdigit(token)) {
                double value;
                input >> value;
                values.push(value);
            } else if (token == '+' || token == '-' || token == '*' || token == '/' || token == '%') {
                while (!ops.empty() && precedence(ops.top()) >= precedence(token)) {
                    applyOperator(ops, values);
                }
                ops.push(input.get());
            } else if (token == '(') {
                ops.push(input.get());
            } else if (token == ')') {
                while (!ops.empty() && ops.top() != '(') {
                    applyOperator(ops, values);
                }
                ops.pop();  // Remove the '(' from the stack
                input.get();  // Remove the ')' from the input
            } else {
                input.get();  // Ignore any other characters
            }
        }
        while (!ops.empty()) {
            applyOperator(ops, values);
        }
        return values.top();
    }

    /**
     * @brief Apply the operator to the top values in the stack.
     * @param ops Stack of operators.
     * @param values Stack of values.
     */
    static void applyOperator(std::stack<char>& ops, std::stack<double>& values) {
        char op = ops.top();
        ops.pop();
        double right = values.top();
        values.pop();
        double left = values.top();
        values.pop();
        switch (op) {
            case '+': values.push(left + right); break;
            case '-': values.push(left - right); break;
            case '*': values.push(left * right); break;
            case '/': values.push(left / right); break;
            case '%': values.push(fmod(left, right)); break;
        }
    }

    /**
     * @brief Get the precedence of the operator.
     * @param op Operator character.
     * @return Precedence of the operator.
     */
    static int precedence(char op) {
        switch (op) {
            case '+':
            case '-': return 1;
            case '*':
            case '/':
            case '%': return 2;
            default: return 0;
        }
    }
};

#endif // TEMPLATETANGO_H
