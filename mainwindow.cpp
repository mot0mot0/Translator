#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("Устинов Илья ИС-41");

    textEdit = new QPlainTextEdit(this);
    textEdit->setEnabled(false);

    lexicalTable = new QTableWidget(this);
    precedenceMatrixTable = new QTableWidget(this);
    syntaxTreeWidget = new QTreeWidget(this);

    baseTriadsList = new QListWidget(this);
    foldingTriadsList = new QListWidget(this);
    resultTriadsList = new QListWidget(this);

    loadFileButton = new QPushButton("Выбрать файл", this);

    QWidget *tab1 = new QWidget;
    QVBoxLayout *textInputLayout = new QVBoxLayout(tab1);
    textInputLayout->addWidget(loadFileButton);
    textInputLayout->addWidget(textEdit);
    ui->tabWidget->addTab(tab1, "Исходный текст");

    QWidget *tab2 = new QWidget;
    QVBoxLayout *lexicalLayout = new QVBoxLayout(tab2);
    lexicalLayout->addWidget(lexicalTable);
    ui->tabWidget->addTab(tab2, "Лексический анализатор");

    QWidget *tab3 = new QWidget;
    QVBoxLayout *precedenceMatrixLayout = new QVBoxLayout(tab3);
    precedenceMatrixLayout->addWidget(precedenceMatrixTable);
    ui->tabWidget->addTab(tab3, "Матрица предшествования");

    QWidget *tab4 = new QWidget;
    QVBoxLayout *syntaxTreeLayout = new QVBoxLayout(tab4);
    syntaxTreeLayout->addWidget(syntaxTreeWidget);
    ui->tabWidget->addTab(tab4, "Синтаксическое дерево");

    QWidget *tab5 = new QWidget;
    QHBoxLayout *triadsLayout = new QHBoxLayout(tab5);
    triadsLayout->addWidget(baseTriadsList);
    triadsLayout->addWidget(foldingTriadsList);
    triadsLayout->addWidget(resultTriadsList);
    ui->tabWidget->addTab(tab5, "Триады");

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
        generateCode();

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
    if (parseF(tokens, index, treeNode)) {
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
        TreeNode child = {"node", "F", QVector<TreeNode>()};

        child.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
        index++;
        if (index < tokens.size() && tokens[index].value == "(") {
            child.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
            index++;

            TreeNode tNode = {"node", "T", QVector<TreeNode>()};
            if (parseT(tokens, index, tNode)) {
                child.children.append(tNode);
                if (index < tokens.size() && tokens[index].value == ")") {
                    child.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
                    index++;
                    if (index < tokens.size() && tokens[index].value == "do") {
                        child.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
                        index++;

                        if (parseG(tokens, index, child)) {
                            treeNode.children.append(child);
                            return true;
                        }
                    }
                }
            }
        }
    } else if (parseG(tokens, index, treeNode)) {
        return true;
    }
    if (syntaxError.value == NULL) {
        syntaxError = tokens[index];
    }
    return false;
}

bool MainWindow::parseG(const QList<Token> &tokens, int &index, TreeNode &treeNode) {
    if (index < tokens.size() && tokens[index].value == "for") {
        if (parseF(tokens, index, treeNode)) {
            return true;
        }
        if (syntaxError.value == NULL) {
            syntaxError = tokens[index];
        }
        return false;
    } else if (index + 5 < tokens.size() && tokens[index + 1].type == Assignment && tokens[index + 5].type == Assignment) {
        TreeNode fNode1 = {"node", "F", QVector<TreeNode>()};
        if (parseAssignment(tokens, index, fNode1)) {
            treeNode.children.append(fNode1);
            if (index < tokens.size() && tokens[index].value == ";") {
                treeNode.children.append({"lexeme", tokens[index].value, QVector<TreeNode>()});
                index++;

                TreeNode fNode2 = {"node", "F", QVector<TreeNode>()};
                if (index + 5 < tokens.size() && tokens[index + 1].type == Assignment && tokens[index + 5].type == Assignment) {
                    return parseG(tokens, index, treeNode);
                } else {
                    if (parseAssignment(tokens, index, fNode2)) {
                        treeNode.children.append(fNode2);
                        return true;
                    }
                }
            }
        }
        return false;
    }
    TreeNode fNode = {"node", "F", QVector<TreeNode>()};
    if (parseAssignment(tokens, index, fNode)) {
        treeNode.children.append(fNode);
        return true;
    }
    return false;
}

bool MainWindow::parseT(const QList<Token> &tokens, int &index, TreeNode &treeNode) {
    if (tokens[index].value != ";") {

        if (parseF(tokens, index, treeNode)) {
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
                            return parseF(tokens, index, treeNode);
                        }
                    }
                }
            }
        }
    } else {
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
                        return parseF(tokens, index, treeNode);
                    }
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

QVector<Triad> MainWindow::generateTriads(const TreeNode& node, int& counter, QMap<QString, int>& triadCache) {
    QVector<Triad> triads;

    if (node.type == "lexeme") {
        return triads;
    }

    if (node.value == "F" && !node.children.isEmpty()) {
        if (node.children.size() > 2 && node.children[1].value == ":=") {
            QString leftOperand = node.children[0].value;
            QString rightOperand = node.children[2].value;

            triads.append(Triad(":=", leftOperand, rightOperand));
            triads.last().index = ++counter;
        }
        else if (node.children.size() > 4 && node.children[0].value == "for") {
            TreeNode tNode = node.children[2];

            QVector<Triad> tTriads = generateTriads(tNode, counter, triadCache);
            triads.append(tTriads);

            int conditionIndex = counter - tTriads.size() + 1;

            QVector<Triad> bodyTriads;
            for (const TreeNode& child : node.children) {
                if (child.value == "F") {
                    QVector<Triad> nestedBodyTriads = generateTriads(child, counter, triadCache);
                    bodyTriads.append(nestedBodyTriads);
                }
            }

            triads.append(bodyTriads);

            triads.append(Triad("for",
                                QString("^%1").arg(conditionIndex),
                                QString("^%1").arg(counter)));
            triads.last().index = ++counter;
        }
        else if (node.children.size() > 2 && (node.children[1].value == "++" || node.children[1].value == "--")) {
            QString leftOperand = node.children[0].value;
            QString rightOperand = "1";

            triads.append(Triad(node.children[1].value == "++" ? "+" : "-", leftOperand, rightOperand));
            triads.last().index = ++counter;
        }
    }
    else if (node.value == "T") {
        for (const TreeNode& child : node.children) {
            QVector<Triad> childTriads = generateTriads(child, counter, triadCache);
            triads.append(childTriads);
        }
    }
    else if (node.value == "E" && node.children.size() == 3) {
        QString leftOperand = node.children[0].value;
        QString operation = node.children[1].value;
        QString rightOperand = node.children[2].value;

        triads.append(Triad(operation, leftOperand, rightOperand));
        triads.last().index = ++counter;
    }

    for (auto& triad : triads) {
        if (triad.index == 0) {
            triad.index = ++counter;
        }
    }

    return triads;
}

QVector<Triad> MainWindow::foldTriads(const QVector<Triad>& inputTriads) {
    QVector<Triad> foldedTriads;
    QMap<QString, QString> lastAssignment;

    for (const Triad& triad : inputTriads) {
        Triad newTriad = triad;

        if (newTriad.operation == ":=") {
            QString variable = newTriad.operand1;
            QString value = newTriad.operand2;

            if (lastAssignment.contains(value)) {
                QString assignedValue = lastAssignment[value];

                bool isConst = assignedValue.toInt(nullptr, 10);
                if (isConst) {
                    value = assignedValue;
                }
            }

            lastAssignment[variable] = value;

            newTriad.operand2 = value;
        }

        foldedTriads.append(newTriad);
    }

    for (int i = 0; i < foldedTriads.size(); ++i) {
        foldedTriads[i].index = i + 1;
    }

    return foldedTriads;
}

QVector<Triad> MainWindow::removeRedundantTriads(const QVector<Triad>& inputTriads) {
    QVector<Triad> optimizedTriads;
    QSet<QString> usedVariables;
    QMap<QString, int> lastAssignmentIdx;
    QMap<int, int> indexMapping;

    int currentIndex = 1;

    for (int i = 0; i < inputTriads.size(); ++i) {
        const Triad& triad = inputTriads[i];

        if (triad.operation == ":=") {
            QString variable = triad.operand1;

            if (lastAssignmentIdx.contains(variable) && !usedVariables.contains(variable)) {
                optimizedTriads.removeAt(lastAssignmentIdx[variable]);
            }

            lastAssignmentIdx[variable] = optimizedTriads.size();
        } else {
            if (!triad.operand1.isEmpty()) {
                usedVariables.insert(triad.operand1);
            }
            if (!triad.operand2.isEmpty()) {
                usedVariables.insert(triad.operand2);
            }
        }

        optimizedTriads.append(triad);
        indexMapping[i + 1] = currentIndex++;
    }

    for (Triad& triad : optimizedTriads) {
        if (triad.operand1.startsWith("^")) {
            int oldIndex = triad.operand1.midRef(1).toInt();
            if (indexMapping.contains(oldIndex)) {
                triad.operand1 = QString("^%1").arg(indexMapping[oldIndex]);
            }
        }
        if (triad.operand2.startsWith("^")) {
            int oldIndex = triad.operand2.midRef(1).toInt();
            if (indexMapping.contains(oldIndex)) {
                triad.operand2 = QString("^%1").arg(indexMapping[oldIndex]);
            }
        }
    }

    for (int i = 0; i < optimizedTriads.size(); ++i) {
        optimizedTriads[i].index = i + 1;
    }

    return optimizedTriads;
}


void MainWindow::displayTriads(QListWidget *widget, const QVector<Triad>& triads) {
    widget->clear();
    for (const auto& triad : triads) {
        widget->addItem(QString::number(triad.index) + ". " + triad.toString());
    }
}

void MainWindow::generateCode() {
    int counter = 0;
    QMap<QString, int> triadCache;
    TreeNode rootNode = syntaxTree.children[0];

    QVector<Triad> baseTriads = generateTriads(rootNode, counter, triadCache);
    displayTriads(baseTriadsList, baseTriads);


    QVector<Triad> collapsedTriads = foldTriads(baseTriads);
    displayTriads(foldingTriadsList, collapsedTriads);

    QVector<Triad> optimizedTriads = removeRedundantTriads(collapsedTriads);
    displayTriads(resultTriadsList, optimizedTriads);
}
