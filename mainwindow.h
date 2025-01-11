#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "token.h"
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

    void addLexemToTable(const QString& type, const QString& value, int line, int column);
    QList<Token> lexicalAnalysis(const QString &text);
    void syntaxAnalysis(const QList<Token> &tokens);

    // Функции для синтаксического анализа
    bool parse(const QList<Token> &tokens);
    bool parseS(const QList<Token> &tokens, int &index, QTreeWidgetItem *treePoint);
    bool parseF(const QList<Token> &tokens, int &index, QTreeWidgetItem *treePoint);
    bool parseG(const QList<Token> &tokens, int &index, QTreeWidgetItem *treePoint);
    bool parseAssignment(const QList<Token> &tokens, int &index, QTreeWidgetItem *treePoint);
    bool parseT(const QList<Token> &tokens, int &index, QTreeWidgetItem *treePoint);
    bool parseE(const QList<Token> &tokens, int &index, QTreeWidgetItem *treePoint);
    bool parseH(const QList<Token> &tokens, int &index, QTreeWidgetItem *treePoint);
    bool parseK(const QList<Token> &tokens, int &index, QTreeWidgetItem *treePoint);
};

#endif // MAINWINDOW_H
