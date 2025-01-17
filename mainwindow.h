#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "token.h"
#include "treeNode.h"
#include "triad.h"

#include <iostream>

#include <QMainWindow>
#include <QTableWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QTreeWidget>
#include <QList>
#include <QPair>
#include <QDebug>
#include <QListWidget>
#include <QHash>
#include <QSet>
#include <QMap>
#include <QString>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLoadFile();

private:
    Ui::MainWindow *ui;
    QPlainTextEdit *textEdit;
    QTableWidget *lexicalTable;
    QTableWidget *precedenceMatrixTable;
    QTreeWidget *syntaxTreeWidget;
    QPushButton *loadFileButton;
    TreeNode syntaxTree;
    Token syntaxError;
    QListWidget *baseTriadsList;
    QListWidget *foldingTriadsList;
    QListWidget *resultTriadsList;

    void addLexemToTable(const QString& type, const QString& value, int line, int column);
    QList<Token> lexicalAnalysis(const QString &text);
    void syntaxAnalysis(const QList<Token> &tokens);

    bool parse(const QList<Token> &tokens);
    bool parseS(const QList<Token> &tokens, int &index, TreeNode &treeNode);
    bool parseF(const QList<Token> &tokens, int &index, TreeNode &treeNode);
    bool parseG(const QList<Token> &tokens, int &index, TreeNode &treeNode);
    bool parseAssignment(const QList<Token> &tokens, int &index, TreeNode &treeNode);
    bool parseT(const QList<Token> &tokens, int &index, TreeNode &treeNode);
    bool parseE(const QList<Token> &tokens, int &index, TreeNode &treeNode);

    void buildSyntaxTreeWidget(const TreeNode &node, QTreeWidgetItem *parent);

    QVector<Triad> generateTriads(const TreeNode& node, int& counter, QMap<QString, int>& triadCache);
    QVector<Triad> foldTriads(const QVector<Triad>& inputTriads);
    QVector<Triad> removeRedundantTriads(const QVector<Triad>& triads);
    void displayTriads(QListWidget *widget, const QVector<Triad>& triads);
    void generateCode();
};

#endif // MAINWINDOW_H
