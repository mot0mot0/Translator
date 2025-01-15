#ifndef TOKEN_H
#define TOKEN_H

#include <QString>

// Перечисление типов лексем
enum TokenType {
    Identifier,        // Идентификатор
    Keyword,           // Ключевое слово
    Number,            // Число
    StringConstant,    // Строковая константа
    Operator,          // Арифметический оператор
    Comparison,        // Оператор сравнения
    Assignment,        // Знак присваивания
    SpecialChar,       // Специальный символ
    Error              // Ошибка
};

// Структура для представления лексемы
struct Token {
    TokenType type;
    QString value;
    int line;
    int column;

    Token() {
        value = "";
        clear();
    }

    Token(TokenType type, const QString& value, int line, int column)
        : type(type), value(value), line(line), column(column) {}

    void clear() {
        this->type = Error;
        this->value.clear();
        this->line = 0;
        this->column = 0;
    }
};

#endif // TOKEN_H
