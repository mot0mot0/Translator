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
    TokenType type;    // Тип лексемы
    QString value;     // Значение лексемы
    int line;          // Номер строки, где найдена лексема
    int column;        // Позиция лексемы в строке

    // Конструктор для удобного создания токенов
    Token(TokenType type, const QString& value, int line, int column)
        : type(type), value(value), line(line), column(column) {}
};

#endif // TOKEN_H
