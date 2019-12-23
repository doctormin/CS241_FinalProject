#include "mainwindow.h"
#include "worker1.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    qDebug() << "Main Thread : " << QThread::currentThreadId();

    ui->setupUi(this);

    ui->treeWidget->setColumnCount(3);
    ui->treeWidget->setHeaderLabels(QStringList() << "Name " << "Size" << "Type");
    QHeaderView *head=ui->treeWidget->header();
    head->setSectionResizeMode(QHeaderView::ResizeToContents);
    //将"复选框被勾选"的信号与"更新父子选择状态"的槽函数关联起来
    connect(ui->treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(onTreeItemChanged(QTreeWidgetItem*, int)));
    //以下代码初始化了数据库连接
    if (QSqlDatabase::contains("qt_sql_default_connection"))
    {
        database = QSqlDatabase::database("qt_sql_default_connection");
    }
    else
    {
        database = QSqlDatabase::addDatabase("QSQLITE");
        //以下两句保证了 1.数据库仅仅存在于内存中 2.数据库可以建立多个connection(用于多线程)
        database.setConnectOptions("QSQLITE_OPEN_URI;QSQLITE_ENABLE_SHARED_CACHE");
        database.setDatabaseName("file::memory:");
    }
    //以下代码打开了数据库
    if (!database.open())
    {
        qDebug() << "Error: Failed to connect database." << database.lastError();
    }
    else qDebug() << "opened successfully !";

    //以下代码实现线程的创建
    QThread* t1 = new QThread;
    worker1* w1 = new worker1();
    w1->moveToThread(t1);
    connect(w1, SIGNAL (error(QString)), this, SLOT (errorString(QString)));
    connect(t1, SIGNAL (started()), w1, SLOT (run()));
    connect(w1, SIGNAL (finished()), t1, SLOT (quit()));
    connect(w1, SIGNAL (finished()), w1, SLOT (deleteLater()));
    connect(t1, SIGNAL (finished()), t1, SLOT (deleteLater()));
    t1->start();
}

MainWindow::~MainWindow()
{
    delete ui;
    //删除table(此程序中采用了内存数据库，无需delete)
    /*
    QSqlQuery sql;
    sql.prepare("DROP TABLE METRO_PASSENGERS;");
    if(!sql.exec())
    {
        qDebug() << "Error: Fail to delete table." << sql.lastError();
    }
    else
    {
        qDebug() << "Table deleted!";
    }
    */
    database.close();
}

//以下函数为“load chosen files”按钮按下后的槽函数
void MainWindow::on_pushButton_clicked()
{
    ui->pushButton->setEnabled(false);
    ///Step0: 解析数据（将csv中每一列都单独存起来) （防冻+多线程解析）
    QStringList time_list;
    QStringList lineID_list;
    QStringList stationID_list;
    QStringList deviceID_list;
    QStringList status_list;
    QStringList userID_list;
    QStringList payType_list;
    //将选择的文件载入进行处理

    ///Step1: 建立table
    QSqlQuery sql;
    sql.prepare(
                "CREATE TABLE IF NOT EXISTS METRO_PASSENGERS ("\
                "time       TEXT PRIMARY KEY  NOT NULL,"\
                "lineID     TEXT    NOT NULL,"\
                "stationID  INT     NOT NULL,"\
                "deviceID   INT     NOT NULL,"\
                "status     INT     NOT NULL,"\
                "userID     TEXT    NOT NULL,"\
                "payType    INT     NOT NULL);"
                );

    if(!sql.exec())
    {
        qDebug() << "Error: Fail to create table." << sql.lastError();
    }
    else
    {
        qDebug() << "Table created!";
    }
    ///Step2: 插入数据 （多线程）
    ui->pushButton->setEnabled(true);
}

//AddOrigin实现顶层目录的添加（便于快速全选以及load chosen file中使用迭代器checkstate遍历）
void MainWindow::AddOrigin(QString name)
{
    QTreeWidgetItem *itm = new QTreeWidgetItem(ui->treeWidget); //最顶层的结点
    itm->setText(0, name);
    itm->setText(1, " ");
    itm->setText(2, " ");
    itm->setCheckState(0, Qt::Unchecked);
    AddRoot(itm, "2019.1.07", 0, 29, list);
    AddRoot(itm, "2019.1.08", 30, 59, list);
    AddRoot(itm, "2019.1.09", 60, 89, list);
    AddRoot(itm, "2019.1.10", 90, 119, list);
    AddRoot(itm, "2019.1.11", 120, 149, list);
    AddRoot(itm, "2019.1.12", 150, 179, list);
    AddRoot(itm, "2019.1.13", 180, 209, list);
}
//AddRoot实现文件树父节点的添加（即按照日期分类）
void MainWindow::AddRoot(QTreeWidgetItem *parent, QString name, int start, int end, QList<QFileInfo> &list)
{
    QTreeWidgetItem *itm = new QTreeWidgetItem();
    itm->setText(0, name);
    itm->setText(1, " ");
    itm->setText(2, " ");
    //添加复选框 并初始化为unchecked
    itm->setCheckState(0, Qt::Unchecked);
    for(int i = start; i <= end; i++)
    {
        AddChild(itm , list.at(i).fileName()+"         ", QString::number(list.at(i).size()/1024)+"KB         ", "csv File");
    }
    parent->addChild(itm);
}
//AddChild实现文件树子节点的添加（即.csv文件条目）
void MainWindow::AddChild(QTreeWidgetItem *parent ,QString name, QString size, QString type)
{
    QTreeWidgetItem *itm = new QTreeWidgetItem();

    itm->setText(0, name);
    itm->setText(1, size);
    itm->setText(2, type);
    //添加复选框 并初始化为unchecked
    itm->setCheckState(0, Qt::Unchecked);
    parent->addChild(itm);
}
//onTreeItemChanged实现文件树中任意一个结点被勾选后，其子和其父的状态同步更新（此函数仅仅适用于二层次情况）
void MainWindow::onTreeItemChanged(QTreeWidgetItem * item, int column)
{
    bool is_csv;
    int count = item->childCount(); //返回子项的个数
    if (item->checkState(0) == Qt::Checked) //即该节点被勾选时
    {
        if(count > 0) //即该item有子节点
        {
            for(int i = 0; i < count; i++) //同步其所有子节点的状态
            {
                item->child(i)->setCheckState(column, Qt::Checked);
                is_csv = false;
            }
            //updateParentItem(item); //为了优化性能，对于已知是二层次的情况，可以不调用
        }
        else //即该item没有子节点时 更新其父节点
        {
            updateParentItem(item);
            is_csv = true;
        }
    }
    if (item->checkState(0) == Qt::Unchecked) //即该节点被除去勾选时
    {
        if(count > 0) //即该item有子节点
        {
            for(int i = 0; i < count; i++) //同步其所有子节点的状态
            {
                item->child(i)->setCheckState(column, Qt::Unchecked);
                is_csv = false;
            }
        }
        else //即该item没有子节点
        {
             updateParentItem(item);
             is_csv = true;
        }
    }

    file_chosen_name_list.clear();
    //以下代码负责维护一个QStringList, 负责储存所有被勾选的文件的文件名
    QTreeWidgetItemIterator iterator(ui->treeWidget);
    while(*iterator)
    {
        if(((*iterator)->checkState(0) == Qt::Checked) && ((*iterator)->childCount() == 0))
            file_chosen_name_list << ((*iterator)->text(0));
        ++iterator;
    }
    //removeListSame(file_chosen_name_list);  好像并不需要它(去重）
    parse(file_chosen_name_list);
    qDebug()<< "=================================" <<endl;
    qDebug()<<file_chosen_name_list.count();
    for (int i = 0; i < file_chosen_name_list.count(); i++)
    {
        qDebug() << file_chosen_name_list.at(i);
    }
}
void MainWindow::updateParentItem(QTreeWidgetItem* item)
{
    QTreeWidgetItem *parent = item->parent();
    if(parent == nullptr) return;
    //子节点中被选中的数目
    int nSelectedCount = 0;
    int npSelectedCount = 0;
    //子节点数
    int childCount = parent->childCount();
    //判断有多少个子项被选中
    for (int i = 0; i < childCount; i++)
    {
        QTreeWidgetItem* childItem = parent->child(i);
        if (childItem->checkState(0) == Qt::Checked)
        {
               nSelectedCount++;
        }
        if (childItem->checkState(0) == Qt::PartiallyChecked)
        {
               npSelectedCount++;
        }
    }
    if (nSelectedCount <= 0 && npSelectedCount <= 0)  //如果没有子项被选中，父项设置为未选中状态
            parent->setCheckState(0, Qt::Unchecked);
    else if ((nSelectedCount > 0 && nSelectedCount < childCount)||npSelectedCount > 0)    //如果有部分子项被选中，父项设置为部分选中状态，即用灰色显示
           parent->setCheckState(0, Qt::PartiallyChecked);
    else if (nSelectedCount == childCount)    //如果子项全部被选中，父项则设置为选中状态
           parent->setCheckState(0, Qt::Checked);
    updateParentItem(parent);
}

//以下函数为“choose a folder”按钮按下后的槽函数
void MainWindow::on_pushButton_2_clicked()
{
    QString folder_dir = QFileDialog::getExistingDirectory(this, "Please choose the \"dataset\" folder", QDir::homePath());
    MainWindow::dir.setPath(folder_dir);  //将以QSting形式保存的文件名转换为QDir类型并存在public数据dir中

    //以下三句可以实现entryInfoList仅仅返回.csv文件的文件名,但是实际上dataset目录下也只有.csv文件，这是为了防止误操作
    QStringList filters;
    filters << "*.csv";
    dir.setNameFilters(filters);
    MainWindow::list = dir.entryInfoList();  //QList<QFileInfo> list;//dataset下所有文件的信息

    /*此代码块可以显示dataset下的所有文件的文件绝对路径
    for (int i = 0; i < list.size(); i++)
    {
        qDebug() << "Filename " + QString::number(i) + " = " + list.at(i).filePath();
    }
    */
    //以下代码实现文件树的建立（并包含复选框）
    AddOrigin("dataset");
}
//以下函数解析了文件名
void MainWindow::parse(QStringList &list)
{
     for (int i = 0; i < list.count(); i++)
     {
         QString tmp = list.at(i).simplified();
         tmp.replace(",","");
         list.replace(i, tmp);
     }
}
//以下函数无用
void MainWindow::removeListSame(QStringList &list)
{
    for (int i = 0; i < list.count(); i++)
    {
        for (int k = i + 1; k <  list.count(); k++)
        {
            if ( list.at(i) ==  list.at(k))
            {
                list.removeAt(k);
                k--;
            }
        }
    }
}

void MainWindow::csv_parser()
{

}
