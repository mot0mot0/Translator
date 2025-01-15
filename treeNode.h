#ifndef TREENODE_H
#define TREENODE_H

#include <QString>
#include <QVector>

struct TreeNode {
    QString type;
    QString value;
    QVector<TreeNode> children;

    void clear() {
        this->type.clear();
        this->value.clear();
        this->children.clear();
    }
};

#endif // TREENODE_H
