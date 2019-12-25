#include "mainwindow.h"
#include "worker1.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    times_of_button3_clicked = 0;
    qDebug() << "Main thread : " << QThread::currentThreadId();
    filter_time = true;
    filter_lineID = true;
    filter_stationID = true;
    filter_deviceID = true;
    filter_status = true;
    filter_userID = true;
    filter_payType = true;
    ui->setupUi(this);
    ui->progressBar->hide();
    ui->treeWidget->setColumnCount(3);
    ui->treeWidget->setHeaderLabels(QStringList() << "Name " << "Size" << "Type");
    QHeaderView *head=ui->treeWidget->header();
    head->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->statusbar->addPermanentWidget(ui->progressBar);
    ui->pushButton_5->setEnabled(false);
    //将"复选框被勾选"的信号与"更新父子选择状态"的槽函数关联起来
    connect(ui->treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(onTreeItemChanged(QTreeWidgetItem*, int)));
    //connect(this, SIGNAL(droptable()), this, SLOT(on_droptable()));
    // connect(this, SIGNAL(pushbutton3()), this, SLOT(on_pushButton_3_clicked()));
    connect(this, SIGNAL(choose_finished()), this, SLOT(loadfile_enable()));
    connect(ui->treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(runsql_disable()));
    connect(this, SIGNAL(fileloadingFinished(double)), this, SLOT(onLoadingFinished(double)));
    connect(this, SIGNAL(fileloadingFinished(double)), this, SLOT(loadfile_enable()));
    connect(this, SIGNAL(insertfailed()), this, SLOT(on_insertfailed()));
    connect(this, SIGNAL(LoadingProcessChanged(int, int)), this, SLOT(ChangeStatusBarWhileLoaingFile(int, int)));
    //connect(this, SIGNAL(fileloadingFinished(double)), this, SLOT(on_paser_finished()));
    //以下代码初始化了数据库连接
    if (QSqlDatabase::contains("qt_sql_default_connection"))
    {
        database = QSqlDatabase::database("qt_sql_default_connection");
        qDebug()<< "qt_sql_default_connection esits!";
    }
    else
    {
        database = QSqlDatabase::addDatabase("QSQLITE","main_connection");
        //以下两句保证了 1.数据库仅仅存在于内存中 2.数据库可以建立多个connection(用于多线程)
        database.setConnectOptions("QSQLITE_OPEN_URI;QSQLITE_ENABLE_SHARED_CACHE");
        database.setDatabaseName("file::memory:");
    }
    //以下代码打开了数据库
    if (!database.open())
    {
        qDebug() << "Error: Failed to connect database in main thread" << database.lastError();
    }
    else
    {
        qDebug() << "opened successfully in main thread!";
    }

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
    ui->pushButton_3->setEnabled(false); //在载入文件完成之前，apply是无效的
    ui->pushButton->setEnabled(false);
    ui->progressBar->show();
    time->start();

    ///Step0: 解析数据（将csv中每一列都单独存起来) （防冻+多线程解析）
    QStringList time_list;
    QStringList lineID_list;
    QStringList stationID_list;
    QStringList deviceID_list;
    QStringList status_list;
    QStringList userID_list;
    QStringList payType_list;

    ///Step1: 建立table(数据库已经在构造函数中初始化了)
    if(!database.isOpen())
    {
        if (!database.open())
        {
            qDebug() << "Error: Failed to connect database in main thread" << database.lastError();
        }
        else
        {
            qDebug() << "opened successfully in main thread!";
        }
    }
    if(database.tables().contains("METRO_PASSENGERS")){
        emit droptable();
        qDebug() << "emit droptable()";

     }
    //DROP table if exists
    if(times_of_button3_clicked > 0) model->clear();
    QSqlQuery sql(database);
    if(!sql.prepare("DROP TABLE IF EXISTS METRO_PASSENGERS")) qDebug() << "Drop prepare failed";
    if(!sql.exec()) qDebug() << "Drop failed";
    else qDebug() << "DROP SUCCESSFULY";

    QString creat_time      = "time       TEXT";
    QString creat_lineID    = "lineID     TEXT";
    QString creat_stationID = "stationID  INT";
    QString creat_deviceID  = "deviceID   INT";
    QString creat_status    = "status     INT";
    QString creat_userID    = "userID     TEXT";
    QString creat_payType   = "payType    INT";

    QString query = "CREATE TABLE IF NOT EXISTS METRO_PASSENGERS (";

    bool isFirst = true;

    if(filter_time)
    {
        query = query + creat_time;
        isFirst = false;
    }
    if(filter_lineID)
    {
        if(isFirst)
        {
             query = query + creat_lineID;
             isFirst = false;
        }
        else
             query = query + ", " + creat_lineID;
    }
    if(filter_stationID)
    {
        if(isFirst)
        {
             query = query + creat_stationID;
             isFirst = false;
        }
        else
             query = query + ", " + creat_stationID;
    }
    if(filter_deviceID)
    {
        if(isFirst)
        {
             query = query + creat_deviceID;
             isFirst = false;
        }
        else
             query = query + ", " + creat_deviceID;
    }
    if(filter_status)
    {
        if(isFirst)
        {
             query = query + creat_status;
             isFirst = false;
        }
        else
            query = query + ", " + creat_status;
    }
    if(filter_userID)
    {

        if(isFirst)
        {
             query = query + creat_userID;
             isFirst = false;
        }
        else
             query = query + ", " + creat_userID;
    }
    if(filter_payType)
    {
        if(isFirst)
        {
             query = query + creat_payType;
             isFirst = false;
        }
        else
             query = query + ", " + creat_payType;
    }
    /*
    qDebug()
            << "filter_time" << filter_time
            << "filter_lineID" << filter_lineID
            << "filter_stationID" <<filter_stationID
            << "filter_deviceID" << filter_deviceID
            << "filter_status" << filter_status
            << "filter_userID" << filter_userID
            << "filter_payType" << filter_payType;
    */
    query += ");";
    qDebug() << "+++++++++++++++++++++++++++++++++++++++++++++" << query;

    if(sql.prepare(query))
        qDebug() << "creating table query prepared!";
    else qDebug() << "creating table query not prepared!";

    if(!sql.exec())
    {
        qDebug() << "Error: Fail to create table." << sql.lastError();
    }
    else
    {
        qDebug() << "Table created!";
    }
    ///Step2: 解析数据 （多线程）
    qDebug() << "mainthread" << QThread::currentThreadId();
    QFuture<void> f = QtConcurrent::run(this, &MainWindow::csv_parser);
}
//AddOrigin实现顶层目录的添加（便于快速全选以及load chosen file中使用迭代器checkstate遍历）
void MainWindow::AddOrigin(QString name)
{
    QTreeWidgetItem * filter = new QTreeWidgetItem(ui->treeWidget); //最顶层的结点
    filter->setText(0, "filter(only chosen fields will be loaded into SQL)");
    AddRoot(filter, "load time");
    AddRoot(filter, "load lineID");
    AddRoot(filter, "load stationID");
    AddRoot(filter, "load deviceID");
    AddRoot(filter, "load status");
    AddRoot(filter, "load userID");
    AddRoot(filter, "load payType");
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
void MainWindow::AddRoot(QTreeWidgetItem *parent, QString name)
{
    QTreeWidgetItem *itm = new QTreeWidgetItem();
    itm->setText(0, name);
    itm->setText(1, " ");
    itm->setText(2, " ");
    //添加复选框 并初始化为checked
    itm->setCheckState(0, Qt::Checked);
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
    qDebug() << "Item changed triggered ======================";
    //即改变的item隶属于filter目录下
    if(item->text(0) == "load time")
    {
        if(item->checkState(0) ==  Qt::Checked)
        {
            filter_time = true;
            qDebug() << "filter_time -> True!";
        }
        if(item->checkState(0) ==  Qt::Unchecked)
        {
            filter_time = false;
            qDebug() << "filter_time -> False!";
        }
        return;
    }
    if(item->text(0) ==  "load lineID")
    {
        if(item->checkState(0) ==  Qt::Checked)
        {

            filter_lineID = true;
            qDebug() << "filter_lineID -> True!";
        }
        if(item->checkState(0) ==  Qt::Unchecked)
        {

            filter_lineID = false;
            qDebug() << "filter_lineID -> False!";
        }
        return;
    }
    if(item->text(0) ==  "load stationID")
    {
        if(item->checkState(0) ==  Qt::Checked)
        {

            filter_stationID = true;
            qDebug() << "filter_stationID -> True!";
        }
        if(item->checkState(0) ==  Qt::Unchecked)
        {

            filter_stationID = false;
            qDebug() << "filter_stationID -> False!";
        }
        return;
    }
    if(item->text(0) ==  "load deviceID")
    {
        if(item->checkState(0) ==  Qt::Checked)
        {

            filter_deviceID = true;
            qDebug() << "filter_deviceID -> True!";
        }
        if(item->checkState(0) ==  Qt::Unchecked)
        {

            filter_deviceID = false;
            qDebug() << "filter_diviceID -> False!";
        }
        return;
    }
    if(item->text(0) ==  "load status" )
    {
        if(item->checkState(0) ==  Qt::Checked)
        {

            filter_status = true;
            qDebug() << "filter_status -> True!";
        }
        if(item->checkState(0) ==  Qt::Unchecked)
        {

            filter_status = false;
            qDebug() << "filter_status -> False!";
        }
        return;
    }
    if(item->text(0) ==  "load userID" )
    {
        if(item->checkState(0) ==  Qt::Checked)
        {

            filter_userID = true;
            qDebug() << "filter_userID -> True!";
        }
        if(item->checkState(0) ==  Qt::Unchecked)
        {

            filter_userID = false;
            qDebug() << "filter_userID -> False!";
        }
        return;
    }
    if(item->text(0) == "load payType" )
    {
        if(item->checkState(0) ==  Qt::Checked)
        {

            filter_payType = true;
            qDebug() << "filter_payType -> True!";
        }
        if(item->checkState(0) ==  Qt::Unchecked)
        {

            filter_payType = false;
            qDebug() << "filter_payType -> False!";
        }
        return;
    }

    int count = item->childCount(); //返回子项的个数
    if (item->checkState(0) == Qt::Checked) //即该节点被勾选时
    {
        if(count > 0) //即该item有子节点
        {
            for(int i = 0; i < count; i++) //同步其所有子节点的状态
            {
                item->child(i)->setCheckState(column, Qt::Checked);
            }
            //updateParentItem(item); //为了优化性能，对于已知是二层次的情况，可以不调用
        }
        else //即该item没有子节点时 更新其父节点
        {
            updateParentItem(item);
        }
    }
    if (item->checkState(0) == Qt::Unchecked) //即该节点被除去勾选时
    {
        if(count > 0) //即该item有子节点
        {
            for(int i = 0; i < count; i++) //同步其所有子节点的状态
            {
                item->child(i)->setCheckState(column, Qt::Unchecked);
            }
        }
        else //即该item没有子节点
        {
            updateParentItem(item);
        }
    }
    file_chosen_name_list.clear();
    //以下代码负责维护一个QStringList, 负责储存所有被勾选的文件的文件名
    QTreeWidgetItemIterator iterator(ui->treeWidget);
    while(*iterator)
    {
        if(    ((*iterator)->checkState(0) == Qt::Checked)
            && ((*iterator)->childCount() == 0)
            && ((*iterator)->text(0) != "load time")
            && ((*iterator)->text(0) != "load lineID")
            && ((*iterator)->text(0) != "load stationID")
            && ((*iterator)->text(0) != "load deviceID")
            && ((*iterator)->text(0) != "load status")
            && ((*iterator)->text(0) != "load userID")
            && ((*iterator)->text(0) != "load payType")
           )
        {
            file_chosen_name_list << ((*iterator)->text(0));
        }
        ++iterator;
    }
    //removeListSame(file_chosen_name_list);  好像并不需要它(去重）
    parse(file_chosen_name_list);
    /*
    qDebug()<< "=================================" <<endl;
    qDebug()<<file_chosen_name_list.count();
    for (int i = 0; i < file_chosen_name_list.count(); i++)
    {
        qDebug() << file_chosen_name_list.at(i);
    }
    */
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
    ui->treeWidget->clear();
    qDebug() << ui->pushButton_2->isEnabled();
    MainWindow::folder_dir = QFileDialog::getExistingDirectory(this, "Please choose the \"dataset\" folder", QDir::homePath());
    if(folder_dir == NULL) return;
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
    emit(choose_finished());
}
//以下函数解析了文件名
void MainWindow::parse(QStringList &list)
{
     for (int i = 0; i < list.count(); i++)
     {
         QString tmp = list.at(i).simplified();
         tmp.replace(",","");
         tmp = MainWindow::folder_dir + "/" + tmp;
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
//以下函数完成csv的解析和插入数据库
void MainWindow::csv_parser()
{
    qDebug() << "worker thread : " << QThread::currentThreadId();
    ///连接之前建立的数据库
    if (QSqlDatabase::contains("csv-paser_connection"))
        {
            database = QSqlDatabase::database("csv-paser_connection");
            qDebug() << "(QSqlDatabase::contains(csv-paser_connection))";
        }
    else
    {
        database = QSqlDatabase::addDatabase("QSQLITE", "csv-paser_connection");
        database.setConnectOptions("QSQLITE_OPEN_URI;QSQLITE_ENABLE_SHARED_CACHE");
        database.setDatabaseName("file::memory:");
    }
    if (!database.open())
    {
        qDebug() << "Error: Failed to connect database in csv_parser thread" << database.lastError();
    }
    else qDebug() << "opened successfully in csv_parser thread!";
    //开启事务
    QSqlQuery sql(database);
    if(sql.prepare("BEGIN;"))
        qDebug() << "BEGIN query prepared!";
    else qDebug() << "BEGIN query not prepared!";

    if(!sql.exec())
    {
        qDebug() << "Error: Fail to BEGIN." << sql.lastError();
    }
    else
        qDebug() << "BEGIN!";
    QStringList list;
    QStringList file_chosen_name_list_copy = file_chosen_name_list;

    QString query = "INSERT INTO  METRO_PASSENGERS(";
    bool isFirst = true;
    if(filter_time)
    {
        query = query + "time";
        isFirst = false;
    }
    if(filter_lineID)
    {
        if(isFirst)
        {
             query = query + "lineID";
             isFirst = false;
        }
        else
             query = query + ", lineID";
    }
    if(filter_stationID)
    {
        if(isFirst)
        {
             query = query + "stationID";
             isFirst = false;
        }
        else
             query = query + ", stationID";
    }
    if(filter_deviceID)
    {
        if(isFirst)
        {
             query = query + "deviceID";
             isFirst = false;
        }
        else
             query = query + ", deviceID";
    }
    if(filter_status)
    {
        if(isFirst)
        {
             query = query + "status";
             isFirst = false;
        }
        else
            query = query + ", status";
    }
    if(filter_userID)
    {

        if(isFirst)
        {
             query = query + "userID";
             isFirst = false;
        }
        else
             query = query + ", userID";
    }
    if(filter_payType)
    {
        if(isFirst)
        {
             query = query + "payType";
             isFirst = false;
        }
        else
             query = query + ", payType";
    }
    query += ") VALUES (";
    isFirst = true;
    if(filter_time)
    {
        query = query + ":Time";
        isFirst = false;
    }
    if(filter_lineID)
    {
        if(isFirst)
        {
             query = query + ":LineID";
             isFirst = false;
        }
        else
             query = query + ", :LineID";
    }
    if(filter_stationID)
    {
        if(isFirst)
        {
             query = query + ":StationID";
             isFirst = false;
        }
        else
             query = query + ", :StationID";
    }
    if(filter_deviceID)
    {
        if(isFirst)
        {
             query = query + ":DeviceID";
             isFirst = false;
        }
        else
             query = query + ", :DeviceID";
    }
    if(filter_status)
    {
        if(isFirst)
        {
             query = query + ":Status";
             isFirst = false;
        }
        else
            query = query + ", :Status";
    }
    if(filter_userID)
    {

        if(isFirst)
        {
             query = query + ":UserID";
             isFirst = false;
        }
        else
             query = query + ", :UserID";
    }
    if(filter_payType)
    {
        if(isFirst)
        {
             query = query + ":PayType";
             isFirst = false;
        }
        else
             query = query + ", :PayType";
    }
    query += ");";

    sql.prepare(query);
    qDebug() << "--------------------->" << query;
    bool flag = true;
    for(int i = 0; i < file_chosen_name_list_copy.count(); i++)
    {
        emit(LoadingProcessChanged(i + 1, file_chosen_name_list_copy.count()));
        QFile file(file_chosen_name_list_copy.at(i));
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << "------------------------->files opend failed!";
        }
        QTextStream in(&file);  //QTextStream读取数据
        //loading
        qDebug() << "file" << i << " loading....";
        in.readLine();

        /*
        qDebug()
                << "filter_time" << filter_time
                << "filter_lineID" << filter_lineID
                << "filter_stationID" <<filter_stationID
                << "filter_deviceID" << filter_deviceID
                << "filter_status" << filter_status
                << "filter_userID" << filter_userID
                << "filter_payType" << filter_payType;
        */
        while(!in.atEnd())
           {
              QString fileLine = in.readLine();  //从第一行读取至下一行
              list = fileLine.split(",", QString::SkipEmptyParts);
              /*
              for (i = 0; i < list.count(); i++)
                  qDebug() << list.at(i);
              */
              if(filter_time) sql.bindValue(":Time", list.at(0));//qDebug() << "bind :Time to " << list.at(0);}
              if(filter_lineID) sql.bindValue(":LineID", list.at(1)); //qDebug() << "bind :LineID to " << list.at(1);}
              if(filter_stationID) sql.bindValue(":StationID", list.at(2)); //qDebug() << "bind :StationID to "<<  list.at(2);}
              if(filter_deviceID) sql.bindValue(":DeviceID", list.at(3)); //qDebug() << "bind :DeviceID to " << list.at(3);}
              if(filter_status) sql.bindValue(":Status", list.at(4)); //qDebug() << "bind :Status to " <<  list.at(4);}
              if(filter_userID) sql.bindValue(":UserID", list.at(5));// qDebug() << "bind :UserID to " << list.at(5);}
              if(filter_payType) sql.bindValue(":PayType", list.at(6)); //qDebug() << "bind :PayType to " << list.at(6);}

              if(!sql.exec())
              {
                  if(flag == true)
                  {
                      emit(insertfailed());
                      qDebug() << sql.lastError();
                  }
              }

           }
        file.close();
    }
    //COMMIT
    if(sql.prepare("COMMIT;"))
        qDebug() << "COMMIT query prepared!";
    else qDebug() << "COMMIT query not prepared!";
    if(!sql.exec())
    {
        qDebug() << "Error: Fail to COMMIT." << sql.lastError();
    }
    else
        qDebug() << "COMMIT!";

    /*
    QString query = "select * from METRO_PASSENGERS";
    sql.exec(query);
    while(sql.next())
    {
        qDebug() << sql.value(0);
    }
    */
    double Time = time->restart()/1000.0;
    qDebug() << "time is " << Time;
    emit(fileloadingFinished(Time));
    database.close();
}

void MainWindow::loadfile_enable()
{
    ui->pushButton->setEnabled(true);
    qDebug() << "ui->pushButton enabled!";
}

void MainWindow::ChangeStatusBarWhileLoaingFile(int i, int j)
{
    QString status = "loading file " + QString::number(i) + " / " +  QString::number(j) + " ....";
    ui->statusbar->showMessage(status);
    double x = double(i) / j * 100;
    ui->progressBar->setValue(x);
}

void MainWindow::onLoadingFinished(double time)
{
    QString status = "Finished in " + QString::number(time) + "s";
    ui->statusbar->showMessage(status);
    ui->progressBar->hide();
    ui->pushButton_3->setEnabled(true);
    qDebug() << "status Finished!  & Progressbar hided!";
}
//run button
void MainWindow::on_pushButton_3_clicked()
{
    times_of_button3_clicked += 1;
    if(!database.isOpen())
    {
        if (!database.open())
        {
            qDebug() << "Error: Failed to connect database in tableview" << database.lastError();
        }
        else qDebug() << "opened successfully in tableview!";
    }
    else qDebug() << "opened successfully in tableview!";
    model = new QSqlQueryModel();
    QString query = ui->textEdit->toPlainText();
    model->setQuery(query, database);
    ui->tableView->setModel(model);
    ui->tableView->show();
    //while(model->canFetchMore()) model->fetchMore();
}
/*
void MainWindow::on_droptable()
{
    if(!database.isOpen())
    {
        if (!database.open())
        {
            qDebug() << "Error: Failed to connect database in on_droptable" << database.lastError();
        }
        else qDebug() << "opened successfully in on_droptable!";
    }
    ui->textEdit->setText("DROP TABLE IF EXISTS METRO_PASSENGERS");
    emit(pushbutton3());
}

void MainWindow::on_paser_finished()
{
    ui->textEdit->setText("SELECT * FROM METRO_PASSENGERS");
    emit(pushbutton3());
}
*/
void MainWindow::on_insertfailed()
{
    ui->progressBar->hide();
    ui->statusbar->showMessage("Oooops! inserting failed PLEASE drop table METRO_PASSENGER explicity in \"SQL tab widgets\" ");
}
//若在改变文件树选择尚未点击load file则run无法执行
void MainWindow::runsql_disable()
{
    ui->pushButton_3->setEnabled(false);\
    ui->statusbar->showMessage("Please load newly chosen files first.");
    qDebug() << "ui->pushButton_3 disabled!";
}
//load map
void MainWindow::on_pushButton_4_clicked()
{
    QString filter = "CSV Files (*.csv)";
    QString file_name = QFileDialog::getOpenFileName(this, "Please choose the Metro_roadMap.csv", QDir::homePath(), filter);
    qDebug() << file_name;
    //open the file chosen
    QFile file(file_name);
    //file not opened
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::warning(this, "warning", "file not opened");
        return;
    }

    QTextStream in(&file);
    in.readLine(); //第一行不要
    QString fileline;
    QStringList list;
    int j = 0;
    while(!in.atEnd())
    {
        fileline = in.readLine();
        //qDebug() << fileline;
        list = fileline.split(",");
        for(int i = 1; i < 82; i++)
            MainWindow::matrix[j][i-1] = list[i].toInt();
        j++;

    }
    /*
    for(int i = 0; i < 80; i++)
        for (j = 0; j < 80; j++)
            qDebug() << matrix[i][j];*/
    ui->statusbar->showMessage("Map loaded successfully !");
    ui->pushButton_5->setEnabled(true);
}
//Get the Plan
void MainWindow::on_pushButton_5_clicked()
{

}
