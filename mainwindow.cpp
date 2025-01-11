#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    textEdit = new QPlainTextEdit(this);
    textEdit->setEnabled(false);

    lexicalTable = new QTableWidget(this);
    precedenceMatrixTable = new QTableWidget(this);
    syntaxTreeWidget = new QTreeWidget(this);

    loadFileButton = new QPushButton("Выбрать файл", this);

    // Вкладка 1: Исходный текст
    QWidget *tab1 = new QWidget;
    QVBoxLayout *textInputLayout = new QVBoxLayout(tab1);
    textInputLayout->addWidget(loadFileButton);
    textInputLayout->addWidget(textEdit);
    ui->tabWidget->addTab(tab1, "Исходный текст");

    // Вкладка 2: Лексический анализатор
    QWidget *tab2 = new QWidget;
    QVBoxLayout *lexicalLayout = new QVBoxLayout(tab2);
    lexicalLayout->addWidget(lexicalTable);
    ui->tabWidget->addTab(tab2, "Лексический анализатор");

    // Вкладка 3: Матрица предшествования
    QWidget *tab3 = new QWidget;
    QVBoxLayout *precedenceMatrixLayout = new QVBoxLayout(tab3);
    precedenceMatrixLayout->addWidget(precedenceMatrixTable);
    ui->tabWidget->addTab(tab3, "Матрица предшествования");

    // Вкладка 4: Синтаксическое дерево
    QWidget *tab4 = new QWidget;
    QVBoxLayout *syntaxTreeLayout = new QVBoxLayout(tab4);
    syntaxTreeLayout->addWidget(syntaxTreeWidget);
    ui->tabWidget->addTab(tab4, "Синтаксическое дерево");

    lexicalTable->setColumnCount(4);
    lexicalTable->setHorizontalHeaderLabels({"Тип", "Значение", "Строка", "Столбец"});
    lexicalTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    lexicalTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    lexicalTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    precedenceMatrixTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    precedenceMatrixTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    syntaxTreeWidget->setHeaderHidden(true);

    connect(loadFileButton, &QPushButton::clicked, this, &MainWindow::onLoadFile);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onLoadFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Выбрать файл", "", "Текстовые файлы (*.txt);;Все файлы (*.*)");
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл");
        return;
    }

    QString content = file.readAll();
    file.close();
    textEdit->setPlainText(content);
    syntaxTreeWidget->clear();

    QList<Token> tokens = lexicalAnalysis(content);

    if (parse(tokens)) {
        QMessageBox::information(this, "Синтаксический анализ", "Анализ успешно завершен!");
        syntaxAnalysis(tokens);
        syntaxTreeWidget->expandAll();
    } else {
        QMessageBox::critical(this, "Синтаксический анализ", "Ошибка синтаксического анализа!");
    }
}

QList<Token> MainWindow::lexicalAnalysis(const QString &text)
{
    QStringList lines = text.split('\n');
    lexicalTable->setRowCount(0);

    QList<Token> tokens;

    QRegularExpression identifierRegex(R"(^[a-zA-Z_][a-zA-Z0-9_]*)");
    QRegularExpression numberRegex(R"(^\d+)");
    QRegularExpression stringRegex(R"(^"[^"]*")");
    QRegularExpression operatorRegex(R"(^[+\-*/])");
    QRegularExpression comparisonRegex(R"(^[<>=])");
    QRegularExpression assignmentRegex(R"(^:=)");
    QRegularExpression specialCharRegex(R"(^[\(\)\{\};])");
    QStringList keywords = {"for", "do"};

    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i];
        int column = 0;

        while (column < line.length()) {
            QStringRef subLine = line.midRef(column).trimmed();

            if (subLine.startsWith("//")) {
                break;
            }

            if (line[column].isSpace()) {
                column++;
                continue;
            }

            // Ключевые слова
            bool keywordMatched = false;
            for (const QString &keyword : keywords) {
                if (subLine.startsWith(keyword) &&
                    (subLine.size() == keyword.length() || !subLine[keyword.length()].isLetterOrNumber())) {
                    addLexemToTable("Ключевое слово", keyword, i + 1, column + 1);
                    tokens.append({Keyword, keyword, i + 1, column + 1});
                    column += keyword.length();
                    keywordMatched = true;
                    break;
                }
            }
            if (keywordMatched) {
                continue;
            }

            // Строковые константы
            QRegularExpressionMatch match = stringRegex.match(line.mid(column));
            if (match.hasMatch()) {
                addLexemToTable("Строковая константа", match.captured(), i + 1, column + 1);
                tokens.append({StringConstant, match.captured(), i + 1, column + 1});
                column += match.capturedLength();
                continue;
            }

            // Знаки присваивания
            match = assignmentRegex.match(line.mid(column));
            if (match.hasMatch()) {
                addLexemToTable("Знак присваивания", match.captured(), i + 1, column + 1);
                tokens.append({Assignment, match.captured(), i + 1, column + 1});
                column += match.capturedLength();
                continue;
            }

            // Арифметические операторы
            match = operatorRegex.match(line.mid(column));
            if (match.hasMatch()) {
                addLexemToTable("Арифметический оператор", match.captured(), i + 1, column + 1);
                tokens.append({Operator, match.captured(), i + 1, column + 1});
                column += match.capturedLength();
                continue;
            }

            // Операторы сравнения
            match = comparisonRegex.match(line.mid(column));
            if (match.hasMatch()) {
                addLexemToTable("Оператор сравнения", match.captured(), i + 1, column + 1);
                tokens.append({Comparison, match.captured(), i + 1, column + 1});
                column += match.capturedLength();
                continue;
            }

            // Числа
            match = numberRegex.match(line.mid(column));
            if (match.hasMatch()) {
                addLexemToTable("Число", match.captured(), i + 1, column + 1);
                tokens.append({Number, match.captured(), i + 1, column + 1});
                column += match.capturedLength();
                continue;
            }

            // Идентификаторы
            match = identifierRegex.match(line.mid(column));
            if (match.hasMatch()) {
                QString tokenValue = match.captured();

                if (keywords.contains(tokenValue)) {
                    addLexemToTable("Ключевое слово", tokenValue, i + 1, column + 1);
                    tokens.append({Keyword, tokenValue, i + 1, column + 1});
                } else {
                    addLexemToTable("Идентификатор", tokenValue, i + 1, column + 1);
                    tokens.append({Identifier, tokenValue, i + 1, column + 1});
                }

                column += match.capturedLength();
                continue;
            }

            // Скобки и символы
            match = specialCharRegex.match(line.mid(column));
            if (match.hasMatch()) {
                addLexemToTable("Специальный символ", match.captured(), i + 1, column + 1);
                tokens.append({SpecialChar, match.captured(), i + 1, column + 1});
                column += match.capturedLength();
                continue;
            }

            // Неопознанный символ
            addLexemToTable("Error", QString("Неопознанный символ: %1").arg(line[column]), i + 1, column + 1);
            tokens.append({Error, QString("Неопознанный символ: %1").arg(line[column]), i + 1, column + 1});
            column++;
        }
    }

    return tokens;
}

void MainWindow::addLexemToTable(const QString &type, const QString &value, int line, int column)
{
    int row = lexicalTable->rowCount();
    lexicalTable->insertRow(row);
    lexicalTable->setItem(row, 0, new QTableWidgetItem(type));
    lexicalTable->setItem(row, 1, new QTableWidgetItem(value));
    lexicalTable->setItem(row, 2, new QTableWidgetItem(QString::number(line)));
    lexicalTable->setItem(row, 3, new QTableWidgetItem(QString::number(column)));

    lexicalTable->item(row, 1)->setTextAlignment(Qt::AlignCenter);
    lexicalTable->item(row, 2)->setTextAlignment(Qt::AlignCenter);
    lexicalTable->item(row, 3)->setTextAlignment(Qt::AlignCenter);
}

bool MainWindow::parse(const QList<Token> &tokens)
{
    QTreeWidgetItem *treeRoot = new QTreeWidgetItem(syntaxTreeWidget);
    treeRoot->setText(0, "a");

    int index = 0;
    return parseS(tokens, index, treeRoot) && index == tokens.size();
}

bool MainWindow::parseS(const QList<Token> &tokens, int &index, QTreeWidgetItem *treePoint)
{
    QTreeWidgetItem *treeItemHead = new QTreeWidgetItem(treePoint);
    treeItemHead->setText(0, "a");

    if (parseF(tokens, index, treeItemHead)) {
        if (index < tokens.size() && tokens[index].value == ";") {
            QTreeWidgetItem *treeItem_1 = new QTreeWidgetItem(treePoint);
            treeItem_1->setText(0, tokens[index].value);

            index++;
            return true;
        }
    }
    return false;
}

bool MainWindow::parseF(const QList<Token> &tokens, int &index, QTreeWidgetItem *treePoint)
{
    if (index < tokens.size() && tokens[index].value == "for") {
        QTreeWidgetItem *treeItem_1 = new QTreeWidgetItem(treePoint);
        treeItem_1->setText(0, tokens[index].value);

        index++;
        if (index < tokens.size() && tokens[index].value == "(") {
            QTreeWidgetItem *treeItem_2 = new QTreeWidgetItem(treePoint);
            treeItem_2->setText(0, tokens[index].value);

            index++;

            QTreeWidgetItem *treeItemHead_1 = new QTreeWidgetItem(treePoint);
            treeItemHead_1->setText(0, "a");

            if (parseT(tokens, index, treeItemHead_1)) {
                if (index < tokens.size() && tokens[index].value == ")") {
                    QTreeWidgetItem *treeItem_3 = new QTreeWidgetItem(treePoint);
                    treeItem_3->setText(0, tokens[index].value);

                    index++;
                    if (index < tokens.size() && tokens[index].value == "do") {
                        QTreeWidgetItem *treeItem_4 = new QTreeWidgetItem(treePoint);
                        treeItem_4->setText(0, tokens[index].value);

                        index++;

                        if (index + 1 < tokens.size()){
                            QTreeWidgetItem *treeItemHead_2 = new QTreeWidgetItem(treePoint);
                            treeItemHead_2->setText(0, "a");

                            return parseG(tokens, index, treeItemHead_2);
                        }
                        else {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

bool MainWindow::parseG(const QList<Token> &tokens, int &index, QTreeWidgetItem *treePoint)
{
    return parseF(tokens, index, treePoint) || parseAssignment(tokens, index, treePoint);
}

bool MainWindow::parseAssignment(const QList<Token> &tokens, int &index, QTreeWidgetItem *treePoint)
{
    if (index < tokens.size() && tokens[index].type == Identifier) {
        QTreeWidgetItem *treeItem_1 = new QTreeWidgetItem(treePoint);
        treeItem_1->setText(0, tokens[index].value);

        index++;
        if (index < tokens.size() && tokens[index].value == ":=") {
            QTreeWidgetItem *treeItem_2 = new QTreeWidgetItem(treePoint);
            treeItem_2->setText(0, tokens[index].value);

            index++;
            if (index < tokens.size() &&
                (tokens[index].type == Identifier || tokens[index].type == StringConstant)) {
                QTreeWidgetItem *treeItem_3 = new QTreeWidgetItem(treePoint);
                treeItem_3->setText(0, tokens[index].value);

                index++;
                return true;
            }
        }
    }
    return false;
}

bool MainWindow::parseT(const QList<Token> &tokens, int &index, QTreeWidgetItem *treePoint)
{
    if (tokens[index].value != ";") {
        QTreeWidgetItem *treeItemHead_1 = new QTreeWidgetItem(treePoint);
        treeItemHead_1->setText(0, "a");

        if (parseE(tokens, index, treeItemHead_1)) {
                if (tokens[index].value == ";") {
                    QTreeWidgetItem *treeItem_1 = new QTreeWidgetItem(treePoint);
                    treeItem_1->setText(0, tokens[index].value);

                    index++;

                    QTreeWidgetItem *treeItemHead_2 = new QTreeWidgetItem(treePoint);
                    treeItemHead_2->setText(0, "a");

                    bool parseHResult = parseH(tokens, index, treeItemHead_2);

                    QTreeWidgetItem *treeItem_2 = new QTreeWidgetItem(treePoint);
                    treeItem_2->setText(0, tokens[index].value);

                    QTreeWidgetItem *treeItemHead_3 = new QTreeWidgetItem(treePoint);
                    treeItemHead_3->setText(0, "a");

                    return parseHResult && tokens[index].value == ";" && parseK(tokens, ++index, treeItemHead_3);
                }
            }
    } else {
        QTreeWidgetItem *treeItem_2 = new QTreeWidgetItem(treePoint);
        treeItem_2->setText(0, tokens[index].value);

        index++;

        QTreeWidgetItem *treeItemHead_4 = new QTreeWidgetItem(treePoint);
        treeItemHead_4->setText(0, "a");

        if (parseH(tokens, index, treeItemHead_4)) {
            if (tokens[index].value == ";") {
                QTreeWidgetItem *treeItem_3 = new QTreeWidgetItem(treePoint);
                treeItem_3->setText(0, tokens[index].value);

                index++;

                QTreeWidgetItem *treeItemHead_5 = new QTreeWidgetItem(treePoint);
                treeItemHead_5->setText(0, "a");

                return parseK(tokens, index, treeItemHead_5);
            }
        }
    }
    return false;
}

bool MainWindow::parseE(const QList<Token> &tokens, int &index, QTreeWidgetItem *treePoint)
{
    if (index < tokens.size() && tokens[index].type == Identifier) {
        QTreeWidgetItem *treeItem_1 = new QTreeWidgetItem(treePoint);
        treeItem_1->setText(0, tokens[index].value);

        index++;
        if (index < tokens.size() && tokens[index].value == ":=") {
            QTreeWidgetItem *treeItem_2 = new QTreeWidgetItem(treePoint);
            treeItem_2->setText(0, tokens[index].value);

            index++;
            if (index < tokens.size() && tokens[index].type == Number) {
                QTreeWidgetItem *treeItem_3 = new QTreeWidgetItem(treePoint);
                treeItem_3->setText(0, tokens[index].value);

                index++;
                return true;
            }
        }
    }
    return false;
}

bool MainWindow::parseH(const QList<Token> &tokens, int &index, QTreeWidgetItem *treePoint)
{
    if (index < tokens.size() && tokens[index].type == Identifier) {
        QTreeWidgetItem *treeItem_1 = new QTreeWidgetItem(treePoint);
        treeItem_1->setText(0, tokens[index].value);

        index++;
        if (index < tokens.size() &&
            (tokens[index].value == "<" || tokens[index].value == ">" || tokens[index].value == "=")) {
            QTreeWidgetItem *treeItem_2 = new QTreeWidgetItem(treePoint);
            treeItem_2->setText(0, tokens[index].value);

            index++;
            if (index < tokens.size() && tokens[index].type == Number) {
                QTreeWidgetItem *treeItem_3 = new QTreeWidgetItem(treePoint);
                treeItem_3->setText(0, tokens[index].value);

                index++;
                return true;
            }
        }
    }
    return false;
}

bool MainWindow::parseK(const QList<Token> &tokens, int &index, QTreeWidgetItem *treePoint)
{
    if (index < tokens.size() &&
        ((tokens[index].value == "+" && tokens[index+1].value == "+") || (tokens[index].value == "-" && tokens[index+1].value == "-"))) {
        QTreeWidgetItem *treeItem_1 = new QTreeWidgetItem(treePoint);
        treeItem_1->setText(0, QString(tokens[index].value + tokens[index + 1].value));

        index = index + 2;
        if (index < tokens.size() && tokens[index].type == Identifier) {
            QTreeWidgetItem *treeItem_2 = new QTreeWidgetItem(treePoint);
            treeItem_2->setText(0, tokens[index].value);

            index++;
            return true;
        }
    }
    return false;
}

void MainWindow::syntaxAnalysis(const QList<Token> &tokens)
{
    QList<QString> lexemes;
    QList<QString> lexemesTypes;
    for (const Token &token : tokens) {
        QString type = token.type == 0 ? "id" : token.type == 2 ? "num" : token.type == 3 ? "str" : "def";
        QString value = token.value;

        lexemes.append(value);
        lexemesTypes.append(type);
    }

    precedenceMatrixTable->setRowCount(lexemes.size());
    precedenceMatrixTable->setColumnCount(lexemes.size());
    precedenceMatrixTable->setHorizontalHeaderLabels(lexemes);
    precedenceMatrixTable->setVerticalHeaderLabels(lexemes);

    QMap<QString, QMap<QString, QString>> precedenceRules = {
        {"for", {{"(", "<"}, {"id", "<"}, {"do", ">"}, {";", ">"}}},
        {"do", {{"for", "<"}, {";", ">"}, {")", ">"}}},
        {"id", {{"id", ">"}, {":=", "<"}, {"num", ">"}, {"str", ">"}, {"+", ">"}, {"-", ">"}, {"*", ">"},
                 {"/", ">"}, {"%", ">"}, {"<", ">"}, {">", ">"}, {"<=", ">"}, {">=", ">"}, {"==", ">"},
                 {"!=", ">"}, {"(", ">"}, {")", ">"}, {";", ">"}, {"do", ">"}}},
        {":=", {{"id", "<"}, {"num", "<"}, {"str", "<"}, {"(", "<"}}},
        {"num", {{";", ">"}, {"+", ">"}, {"-", ">"}, {"*", ">"}, {"/", ">"}, {"%", ">"}, {"<", ">"},
                  {">", ">"}, {"<=", ">"}, {">=", ">"}, {"==", ">"}, {"!=", ">"}, {")", ">"}, {"do", ">"}}},
        {"str", {{";", ">"}, {"+", ">"}, {"do", ">"}, {")", ">"}, {"id", ">"}}},
        {"+", {{"id", "<"}, {"num", "<"}, {"str", "<"}, {"(", "<"}}},
        {"-", {{"id", "<"}, {"num", "<"}, {"str", "<"}, {"(", "<"}}},
        {"*", {{"id", "<"}, {"num", "<"}, {"(", "<"}}},
        {"/", {{"id", "<"}, {"num", "<"}, {"(", "<"}}},
        {"%", {{"id", "<"}, {"num", "<"}, {"(", "<"}}},
        {"<", {{"id", "<"}, {"num", "<"}, {"(", "<"}}},
        {">", {{"id", "<"}, {"num", "<"}, {"(", "<"}}},
        {"<=", {{"id", "<"}, {"num", "<"}, {"(", "<"}}},
        {">=", {{"id", "<"}, {"num", "<"}, {"(", "<"}}},
        {"==", {{"id", "<"}, {"num", "<"}, {"(", "<"}}},
        {"!=", {{"id", "<"}, {"num", "<"}, {"(", "<"}}},
        {"(", {{"for", "<"}, {"id", "<"}, {"num", "<"}, {"str", "<"}, {"(", "<"}}},
        {")", {{";", ">"}, {"+", ">"}, {"-", ">"}, {"*", ">"}, {"/", ">"}, {"%", ">"}, {"<", ">"},
                 {">", ">"}, {"<=", ">"}, {">=", ">"}, {"==", ">"}, {"!=", ">"}, {"do", ">"}, {")", ">"}}},
        {";", {{"for", "<"}, {"id", "<"}, {"do", "<"}, {")", ">"}, {";", ">"}}}
    };

    for (int row = 0; row < lexemes.size(); ++row) {
        for (int col = 0; col < lexemes.size(); ++col) {
            const QString &rowLexeme = lexemesTypes[row] == "def" ? lexemes[row] : lexemesTypes[row];
            const QString &colLexeme = lexemesTypes[col] == "def" ? lexemes[col] : lexemesTypes[col];

            if (precedenceRules.contains(rowLexeme) && precedenceRules[rowLexeme].contains(colLexeme)) {
                precedenceMatrixTable->setItem(row, col, new QTableWidgetItem(precedenceRules[rowLexeme][colLexeme]));
                precedenceMatrixTable->item(row, col)->setTextAlignment(Qt::AlignCenter);
            } else {
                precedenceMatrixTable->setItem(row, col, new QTableWidgetItem(" "));
            }
        }
    }
}
