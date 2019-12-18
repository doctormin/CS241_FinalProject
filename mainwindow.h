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

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE



class MainWindow : public QMainWindow
{
    Q_OBJECT

//    void AddRoot(QString name, QString Description);
//    void AddChild(QTreeWidgetItem *parent,QString name, QString Description);
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    QFileSystemModel *dirmodel;
};
#endif // MAINWINDOW_H
