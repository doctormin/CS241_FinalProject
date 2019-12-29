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
#include <QSqlRecord>
#include <QtConcurrent>
#include <QFuture>
#include <QTableView>
#include <QSqlQueryModel>
#include <QtCharts>
#include <QDateTimeEdit>
#include <QMessageBox>
QT_CHARTS_USE_NAMESPACE
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
    void AddRoot(QTreeWidgetItem *parent, QString name);
    void AddChild(QTreeWidgetItem *parent,QString name, QString size, QString type);
    //用于更新父节点的checkstate(当子节点改变时）
    void updateParentItem(QTreeWidgetItem* item);
    void removeListSame(QStringList &list); //并不需要它
    void csv_parser();


private:
    QDir dir; //储存dataset文件夹的路径
    long long MAX;
    QString folder_dir; //储存dataset文件夹的路径
    QSqlQueryModel *model;
    int times_of_button3_clicked;
    bool matrix[81][81];
    bool filter_time;
    bool filter_lineID;
    bool filter_stationID;
    bool filter_deviceID;
    bool filter_status;
    bool filter_userID;
    bool filter_payType;
    QList<QFileInfo> list;//dataset下所有文件的信息
    QStringList file_chosen_name_list;//储存所有被勾选的文件的文件名
    QStringList days_chosen_list; //储存所有被勾选的日期
    QSqlDatabase database;  //储存信息的数据库
    QTime *time = new QTime();
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
signals:
    void choose_finished();
    void fileloadingFinished(double time1, double time2);
    void LoadingProcessChanged(int, int);
    void droptable();
    void insertfailed();
    void building_index_of_sql(double time); //建立索引
    //void OutFlowpoint(long long, long long, bool);
    //void OutFlowpoint_diffline(long long, long long, bool);
    void OutFlow_diffline_finished(bool);
    //void pushbutton3();
    void plot_finished();
    void payType_plot_finished(int, int, int, int);
    void pushbutton3();

private slots:
    void on_pushButton_clicked();
    //void on_paser_finished();
    //void on_droptable();
    void on_pushButton_2_clicked();
    void loadfile_enable();
    void runsql_disable();
    void ChangeStatusBarWhileLoaingFile(int, int);
    void onTreeItemChanged(QTreeWidgetItem * item, int column);
    void onLoadingFinished(double time1, double time2);
    void on_pushButton_3_clicked();
    void on_insertfailed();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
    void on_pushButton_6_clicked();
    void on_building_index_of_sql(double time);
    //void plot(long long ,long long, bool);
    void on_plot_finished();
    void on_checkBox_stateChanged(int arg1);
    void on_payType_plot_finished(int, int, int, int);

    void on_Type_of_analyze_activated(const QString &arg1);
    void on_checkBox_2_stateChanged(int arg1);
    //void on_OutFlowpoint_diffline(long long, long long, bool);
    void on_OutFlow_diffline_finished(bool);

private:
    Ui::MainWindow *ui;
    QChart* chart;
    QSplineSeries *series;
    QSplineSeries *series_in;
    QSplineSeries *series_diffline;
    QSplineSeries *series_diffline_in;
    QPieSeries *payType_series;

};
#endif // MAINWINDOW_H
