#ifndef TRIAD_H
#define TRIAD_H

#include <QString>

struct Triad {
    int index;
    QString operation;
    QString operand1;
    QString operand2;

    Triad() : index(0), operation(""), operand1(""), operand2("") {}

    Triad(const QString& op, const QString& op1, const QString& op2)
        : index(0), operation(op), operand1(op1), operand2(op2) {}

    QString toString() const {
        return QString("%1 [%2, %3]").arg(operation, operand1, operand2);
    }
};

#endif // TRIAD_H
