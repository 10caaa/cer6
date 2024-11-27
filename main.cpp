#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <stdexcept>
#include <memory>
#include <cctype>

enum class TokenType {
    INTEGER,
    PLUSOPERATOR,
    MINUSOPERATOR,
    STAROPERATOR,
    SLASHOPERATOR,
    LPARENTHESIS,
    RPARENTHESIS
};

class Token {
public:
    Token(TokenType type, std::string value) : type(type), value(std::move(value)) {}

    std::string getValue() const { return value; }
    TokenType getType() const { return type; }
    bool isType(TokenType t) const { return type == t; }
    bool isOperator() const {
        return type == TokenType::PLUSOPERATOR || type == TokenType::MINUSOPERATOR ||
               type == TokenType::STAROPERATOR || type == TokenType::SLASHOPERATOR;
    }

private:
    TokenType type;
    std::string value;
};

class Lexer {
public:
    static std::vector<std::shared_ptr<Token>> tokenize(const std::string &expr) {
        std::vector<std::shared_ptr<Token>> tokens;
        size_t i = 0;

        while (i < expr.size()) {
            char ch = expr[i];
            if (isspace(ch)) {
                i++;
                continue;
            }

            if (isdigit(ch)) {
                std::string num;
                while (i < expr.size() && isdigit(expr[i])) {
                    num += expr[i++];
                }
                tokens.emplace_back(std::make_shared<Token>(TokenType::INTEGER, num));
                continue;
            }

            switch (ch) {
                case '+': tokens.emplace_back(std::make_shared<Token>(TokenType::PLUSOPERATOR, "+")); break;
                case '-': tokens.emplace_back(std::make_shared<Token>(TokenType::MINUSOPERATOR, "-")); break;
                case '*': tokens.emplace_back(std::make_shared<Token>(TokenType::STAROPERATOR, "*")); break;
                case '/': tokens.emplace_back(std::make_shared<Token>(TokenType::SLASHOPERATOR, "/")); break;
                case '(': tokens.emplace_back(std::make_shared<Token>(TokenType::LPARENTHESIS, "(")); break;
                case ')': tokens.emplace_back(std::make_shared<Token>(TokenType::RPARENTHESIS, ")")); break;
                default:
                    throw std::runtime_error("Unknown character in expression: " + std::string(1, ch));
            }
            i++;
        }

        return tokens;
    }
};

class Expression {
public:
    virtual int interpret() const = 0;
    virtual ~Expression() = default;
};

class LiteralExpression : public Expression {
public:
    explicit LiteralExpression(int value) : value(value) {}
    int interpret() const override { return value; }

private:
    int value;
};

class BinaryOp : public Expression {
public:
    BinaryOp(std::shared_ptr<Expression> left, std::shared_ptr<Expression> right)
        : left(std::move(left)), right(std::move(right)) {}

protected:
    std::shared_ptr<Expression> left, right;
};

class PlusBinaryOp : public BinaryOp {
public:
    using BinaryOp::BinaryOp;
    int interpret() const override { return left->interpret() + right->interpret(); }
};

class MinusBinaryOp : public BinaryOp {
public:
    using BinaryOp::BinaryOp;
    int interpret() const override { return left->interpret() - right->interpret(); }
};

class StarBinaryOp : public BinaryOp {
public:
    using BinaryOp::BinaryOp;
    int interpret() const override { return left->interpret() * right->interpret(); }
};

class DivBinaryOp : public BinaryOp {
public:
    using BinaryOp::BinaryOp;
    int interpret() const override {
        int denominator = right->interpret();
        if (denominator == 0) {
            throw std::runtime_error("Division by zero");
        }
        return left->interpret() / denominator;
    }
};

class Parser {
public:
    explicit Parser(const std::vector<std::shared_ptr<Token>> &tokens) : tokens(tokens), idx(0) {}

    std::shared_ptr<Expression> parse() {
        return parseExpression();
    }

private:
    const std::vector<std::shared_ptr<Token>> &tokens;
    size_t idx;

    std::shared_ptr<Token> next() {
        if (idx >= tokens.size()) return nullptr;
        return tokens[idx++];
    }

    void rewind() {
        if (idx > 0) idx--;
    }

    std::shared_ptr<Expression> parseExpression() {
        return parseTerm();  // Start by parsing terms
    }

    std::shared_ptr<Expression> parseTerm() {
        auto left = parseFactor();  // Parse the first factor (could be a literal or parenthesis)

        while (true) {
            auto token = next();
            if (!token) break;

            if (token->isType(TokenType::PLUSOPERATOR)) {
                left = std::make_shared<PlusBinaryOp>(left, parseFactor());
            } else if (token->isType(TokenType::MINUSOPERATOR)) {
                left = std::make_shared<MinusBinaryOp>(left, parseFactor());
            } else {
                rewind();
                break;
            }
        }
        return left;
    }

    std::shared_ptr<Expression> parseFactor() {
        auto left = parsePrimary();  // Parse the primary (could be a number or parenthesized expression)

        while (true) {
            auto token = next();
            if (!token) break;

            if (token->isType(TokenType::STAROPERATOR)) {
                left = std::make_shared<StarBinaryOp>(left, parsePrimary());
            } else if (token->isType(TokenType::SLASHOPERATOR)) {
                left = std::make_shared<DivBinaryOp>(left, parsePrimary());
            } else {
                rewind();
                break;
            }
        }
        return left;
    }

    std::shared_ptr<Expression> parsePrimary() {
        auto token = next();
        if (!token) throw std::runtime_error("Unexpected end of expression");

        if (token->isType(TokenType::INTEGER)) {
            return std::make_shared<LiteralExpression>(std::stoi(token->getValue()));
        }

        if (token->isType(TokenType::LPARENTHESIS)) {
            auto expr = parseExpression();
            auto closingParen = next();
            if (!closingParen || !closingParen->isType(TokenType::RPARENTHESIS)) {
                throw std::runtime_error("Expected closing parenthesis");
            }
            return expr;
        }

        throw std::runtime_error("Invalid expression");
    }
};

int main() {
    std::string expr;
    while (true) {
        std::cout << "Enter an expression (or type 'exit' to quit): ";
        std::getline(std::cin, expr);
        
        if (expr == "exit") {
            break;
        }

        try {
            auto tokens = Lexer::tokenize(expr);
            Parser parser(tokens);
            auto expression = parser.parse();
            std::cout << "Result: " << expression->interpret() << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
    return 0;
}
