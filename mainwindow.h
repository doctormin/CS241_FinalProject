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
//#include <QInputDialog>  just for study
#include <QFileSystemModel>
#include <QtDebug>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtConcurrent>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    void parse(QStringList &list); //解析目标csv
    void AddOrigin(QString name);
    void AddRoot(QTreeWidgetItem *paren, QString name, int start, int end, QList<QFileInfo> &list);
    void AddChild(QTreeWidgetItem *parent,QString name, QString size, QString type);
    //用于更新父节点的checkstate(当子节点改变时）
    void updateParentItem(QTreeWidgetItem* item);
    void removeListSame(QStringList &list); //并不需要它
    void csv_parser();
public:
    QDir dir; //储存dataset文件夹的路径
    QList<QFileInfo> list;//dataset下所有文件的信息
    QStringList file_chosen_name_list;//储存所有被勾选的文件的文件名

    QSqlDatabase database;  //储存信息的数据库

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


};
#endif // MAINWINDOW_H
