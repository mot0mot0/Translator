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

    QList<Token> tokens = lexicalAnalysis(content);\

    if (parse(tokens)) {
        QMessageBox::information(this, "Синтаксический анализ", "Анализ успешно завершен!");
        syntaxAnalysis(tokens);

    } else {
        QMessageBox::critical(this, "Синтаксический анализ", QString("Ошибка синтаксического анализа! Лексема %1, строка %2, столбец %3").arg(syntaxError.value, QString::number(syntaxError.line), QString::number(syntaxError.column)));
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
    QRegularExpression operatorRegex(R"(^(\+\+|--))");
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

            QRegularExpressionMatch match = stringRegex.match(line.mid(column));
            if (match.hasMatch()) {
                addLexemToTable("Строковая константа", match.captured(), i + 1, column + 1);
                tokens.append({StringConstant, match.captured(), i + 1, column + 1});
                column += match.capturedLength();
                continue;
            }

            match = assignmentRegex.match(line.mid(column));
            if (match.hasMatch()) {
                addLexemToTable("Знак присваивания", match.captured(), i + 1, column + 1);
                tokens.append({Assignment, match.captured(), i + 1, column + 1});
                column += match.capturedLength();
                continue;
            }

            match = operatorRegex.match(line.mid(column));
            if (match.hasMatch()) {
                addLexemToTable("Арифметический оператор", match.captured(), i + 1, column + 1);
                tokens.append({Operator, match.captured(), i + 1, column + 1});
                column += match.capturedLength();
                continue;
            }

            match = comparisonRegex.match(line.mid(column));
            if (match.hasMatch()) {
                addLexemToTable("Оператор сравнения", match.captured(), i + 1, column + 1);
                tokens.append({Comparison, match.captured(), i + 1, column + 1});
                column += match.capturedLength();
                continue;
            }

            match = numberRegex.match(line.mid(column));
            if (match.hasMatch()) {
                addLexemToTable("Число", match.captured(), i + 1, column + 1);
                tokens.append({Number, match.captured(), i + 1, column + 1});
                column += match.capturedLength();
                continue;
            }

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

            match = specialCharRegex.match(line.mid(column));
            if (match.hasMatch()) {
                addLexemToTable("Специальный символ", match.captured(), i + 1, column + 1);
                tokens.append({SpecialChar, match.captured(), i + 1, column + 1});
                column += match.capturedLength();
                continue;
            }

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

bool MainWindow::parse(const QList<Token> &tokens) {
    syntaxTree.clear();
    syntaxError = Token{Error, NULL, 0, 0};

    TreeNode root = {"node", "S", QVector<TreeNode>()};
    int index = 0;
    if (parseS(tokens, index, root) && index == tokens.size()) {
        syntaxTree = root;
        return true;
    }
    return false;
}

bool MainWindow::parseS(const QList<Token> &tokens, int &index, TreeNode &treeNode) {
    TreeNode child = {"node", "F", QVector<TreeNode>()};\

    if (parseF(tokens, index, child)) {
        treeNode.children.append(child);
        if (index < tokens.size() && tokens[index].value == ";") {
            treeNode.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
            index++;
            return true;
        }
    }
    if (syntaxError.value == NULL) {
        syntaxError = tokens[index];
    }
    return false;
}

bool MainWindow::parseF(const QList<Token> &tokens, int &index, TreeNode &treeNode) {
    if (index < tokens.size() && tokens[index].value == "for") {
        treeNode.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
        index++;
        if (index < tokens.size() && tokens[index].value == "(") {
            treeNode.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
            index++;

            TreeNode tNode = {"node", "T", QVector<TreeNode>()};
            if (parseT(tokens, index, tNode)) {
                treeNode.children.append(tNode);
                if (index < tokens.size() && tokens[index].value == ")") {
                    treeNode.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
                    index++;
                    if (index < tokens.size() && tokens[index].value == "do") {
                        treeNode.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
                        index++;

                        TreeNode fNode = {"node", "F", QVector<TreeNode>()};
                        if (index + 1 < tokens.size())
                        {
                            if (parseF(tokens, index, fNode)) {
                                treeNode.children.append(fNode);
                                return true;
                            }
                        } else {
                            return true;
                        }
                    }
                }
            }
        }
    }
    if (parseAssignment(tokens, index, treeNode)) {
        return  true;
    } else {
        if (syntaxError.value == NULL) {
            syntaxError = tokens[index];
        }
        return false;
    }
}

bool MainWindow::parseT(const QList<Token> &tokens, int &index, TreeNode &treeNode) {
    if (tokens[index].value != ";") {
        TreeNode fNode = {"node", "F", QVector<TreeNode>()};
        if (parseF(tokens, index, fNode)) {
            treeNode.children.append(fNode);
            if (tokens[index].value == ";") {
                treeNode.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
                index++;

                TreeNode eNode = {"node", "E", QVector<TreeNode>()};
                if (parseE(tokens, index, eNode)) {
                    treeNode.children.append(eNode);
                    if (tokens[index].value == ";") {
                        treeNode.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
                        index++;

                        if (tokens[index].value == ")") {
                            return true;
                        } else {
                            TreeNode fNode2 = {"node", "F", QVector<TreeNode>()};
                            if (parseF(tokens, index, fNode2)) {
                                treeNode.children.append(fNode2);
                                return true;
                            }
                        }
                    }
                }
            }
        }
    } else {
        treeNode.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
        index++;

        TreeNode eNode = {"node", "E", QVector<TreeNode>()};
        if (parseE(tokens, index, eNode)) {
            treeNode.children.append(eNode);
            if (tokens[index].value == ";") {
                treeNode.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
                index++;

                TreeNode fNode = {"node", "F", QVector<TreeNode>()};
                if (parseF(tokens, index, fNode)) {
                    treeNode.children.append(fNode);
                    return true;
                }
            }
        }
    }
    if (syntaxError.value == NULL) {
        syntaxError = tokens[index];
    }
    return false;
}

bool MainWindow::parseE(const QList<Token> &tokens, int &index, TreeNode &treeNode) {
    if (index < tokens.size() && tokens[index].type == Identifier) {
        treeNode.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
        index++;
        if (index < tokens.size() && (tokens[index].value == "<" || tokens[index].value == ">" || tokens[index].value == "=")) {
            treeNode.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
            index++;
            if (index < tokens.size() && tokens[index].type == Number) {
                treeNode.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
                index++;
                return true;
            }
        }
    }
    if (syntaxError.value == NULL) {
        syntaxError = tokens[index];
    }
    return false;
}

bool MainWindow::parseAssignment(const QList<Token> &tokens, int &index, TreeNode &treeNode) {
    if (index < tokens.size() && tokens[index].type == Identifier) {
        treeNode.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
        index++;
        if(index < tokens.size() && tokens[index].type == Operator) {
            treeNode.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
            index++;
            return true;
        }
        if (index < tokens.size() && tokens[index].value == ":=") {
            treeNode.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
            index++;
            if (index < tokens.size() && (tokens[index].type == Identifier || tokens[index].type == StringConstant || tokens[index].type == Number)) {
                treeNode.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
                index++;
                return true;
            }
        }
    }
    if (syntaxError.value == NULL) {
        syntaxError = tokens[index];
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

    syntaxTreeWidget->clear();
    if (!syntaxTree.type.isEmpty()) {
        buildSyntaxTreeWidget(syntaxTree, syntaxTreeWidget->invisibleRootItem());
        syntaxTreeWidget->expandAll();
    }
}

void MainWindow::buildSyntaxTreeWidget(const TreeNode &node, QTreeWidgetItem *parent) {
    QTreeWidgetItem *item = new QTreeWidgetItem(parent);
    item->setText(0, QString(node.value));

    for (const TreeNode &child : node.children) {
        buildSyntaxTreeWidget(child, item);
    }

}
