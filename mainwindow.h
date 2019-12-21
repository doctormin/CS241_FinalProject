#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QMessageBox>
#include <QFileDialog>
#include <QTabWidget>
#include <QDir>
#include <QtCore>
#include <QtGui>
#include <QDialog>
#include <QInputDialog> //for study
#include <QFileSystemModel>
#include <QtDebug>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    void AddRoot(QString name, int start, int end, QList<QFileInfo> &list);
    void AddChild(QTreeWidgetItem *parent,QString name, QString size, QString type);
    //用于更新父节点的checkstate(当子节点改变时）
    void updateParentItem(QTreeWidgetItem* item);
    QSqlDatabase database;
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
public slots:
    void onTreeItemChanged(QTreeWidgetItem * item, int column);
private:
    Ui::MainWindow *ui;
    QTreeWidget* tree = new QTreeWidget();

};
#endif // MAINWINDOW_H
