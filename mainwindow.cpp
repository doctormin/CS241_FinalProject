#include "mainwindow.h"
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
    MAX = 0;
    ui->setupUi(this);
    ui->progressBar->hide();
    ui->treeWidget->setColumnCount(3);
    ui->treeWidget->setHeaderLabels(QStringList() << "Name " << "Size" << "Type");
    ui->checkBox->setCheckState(Qt::Unchecked);
    ui->checkBox_2->setCheckState(Qt::Unchecked);
    QHeaderView *head=ui->treeWidget->header();
    head->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->statusbar->addPermanentWidget(ui->progressBar);
    ui->pushButton_5->setEnabled(false);
    /*
    QRegExp rangeforhours("[0-9] | 1[0-9] | 2[0-3]");
    ui->hours_edit->setValidator(new QRegExpValidator(rangeforhours, this));
    QRegExp rangeformins("[0-9] | [1-5][0-9]");
    ui->mins_edit->setValidator(new QRegExpValidator(rangeformins, this));
    */
    chart = new QChart();
    //将"复选框被勾选"的信号与"更新父子选择状态"的槽函数关联起来
    connect(ui->treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(onTreeItemChanged(QTreeWidgetItem*, int)));
    //connect(this, SIGNAL(droptable()), this, SLOT(on_droptable()));
    connect(this, SIGNAL(pushbutton3()), this, SLOT(on_pushButton_3_clicked()));
    connect(this, SIGNAL(choose_finished()), this, SLOT(loadfile_enable()));
    connect(ui->treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(runsql_disable()));
    connect(this, SIGNAL(fileloadingFinished(double, double)), this, SLOT(onLoadingFinished(double, double)));
    connect(this, SIGNAL(fileloadingFinished(double, double)), this, SLOT(loadfile_enable()));
    connect(this, SIGNAL(insertfailed()), this, SLOT(on_insertfailed()));
    connect(this, SIGNAL(LoadingProcessChanged(int, int)), this, SLOT(ChangeStatusBarWhileLoaingFile(int, int)));
    //connect(this, SIGNAL(fileloadingFinished(double)), this, SLOT(on_paser_finished()));
    connect(this, SIGNAL(building_index_of_sql(double)), this, SLOT(on_building_index_of_sql(double)));
    //connect(this, SIGNAL(OutFlowpoint(long long, long long, bool)), this, SLOT(plot(long long, long long, bool)));
    connect(this, SIGNAL(plot_finished()), this, SLOT(on_plot_finished()));
    connect(this, SIGNAL(payType_plot_finished(int, int, int, int)), this, SLOT(on_payType_plot_finished(int, int, int, int)));
    //connect(this, SIGNAL(OutFlowpoint_diffline(long long, long long, bool)), this, SLOT(on_OutFlowpoint_diffline(long long, long long, bool)));
    connect(this, SIGNAL(OutFlow_diffline_finished(bool)), this, SLOT(on_OutFlow_diffline_finished(bool)));
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
    ui->pushButton_6->setDisabled(true);
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
        emit(droptable());
        qDebug() << "emit droptable()";

     }
    //DROP table if exists
    if(times_of_button3_clicked > 0) model->clear();
    QSqlQuery sql(database);
    if(!sql.prepare("DROP TABLE IF EXISTS METRO_PASSENGERS")) qDebug() << "Drop prepare failed";
    if(!sql.exec()) qDebug() << "Drop failed";
    else qDebug() << "DROP SUCCESSFULY";

    QString creat_time      = "time       TEXT";
    QString creat_timestamp = "timestamp  INT";
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
        query = query + creat_time + ", " + creat_timestamp;
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
    //sqDebug() << "+++++++++++++++++++++++++++++++++++++++++++++" << query;

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
    AddRoot(itm, "2019-01-07", 0, 29, list);
    AddRoot(itm, "2019-01-08", 30, 59, list);
    AddRoot(itm, "2019-01-09", 60, 89, list);
    AddRoot(itm, "2019-01-10", 90, 119, list);
    AddRoot(itm, "2019-01-11", 120, 149, list);
    AddRoot(itm, "2019-01-12", 150, 179, list);
    AddRoot(itm, "2019-01-13", 180, 209, list);
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
    if(name == "load time") itm->setDisabled(true);
    if(name == "load stationID") itm->setDisabled(true);
    if(name == "load status") itm->setDisabled(true);
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
    else qDebug() << "database opened successfully in csv_parser thread!";
    //开启事务
    QSqlQuery sql(database);
    sql.prepare("BEGIN;");
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
        query = query + "time, timestamp";
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
        query = query + ":Time, :Timestamp";
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
    //qDebug() << "--------------------->" << query;
    bool flag = true;
    QDateTime tmp;
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
              if(filter_time)
              {
                  sql.bindValue(":Time", list.at(0));
                  //qDebug() << list.at(0);
                  tmp = QDateTime::fromString(list.at(0), "yyyy-MM-dd hh:mm:ss");
                  //qDebug() << tmp;
                  sql.bindValue(":Timestamp",tmp.toUTC().toTime_t());
              }
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
    //建立索引
    double Time1 = time->restart()/1000.0;
    emit(building_index_of_sql(Time1));
    QString buildingIndex = "CREATE INDEX my_index on METRO_PASSENGERS (timestamp, stationID, status)";
    if(filter_lineID) buildingIndex = "CREATE INDEX my_index on METRO_PASSENGERS (timestamp, stationID, status, lineID)";
    sql.prepare(buildingIndex);
    if(!sql.exec())
    {
        qDebug() << "Error: Fail to BUILDING INDEX." << sql.lastError();
    }
    buildingIndex = "CREATE INDEX my_index_2 on METRO_PASSENGERS (timestamp, status)";
    sql.prepare(buildingIndex);
    if(!sql.exec())
    {
        qDebug() << "Error: Fail to BUILDING INDEX_2." << sql.lastError();
    }
    buildingIndex = "CREATE INDEX my_index_3 on METRO_PASSENGERS (timestamp, payType)";
    sql.prepare(buildingIndex);
    if(!sql.exec())
    {
        qDebug() << "Error: Fail to BUILDING INDEX_3." << sql.lastError();
    }
    buildingIndex = "CREATE INDEX my_index_4 on METRO_PASSENGERS (timestamp, stationID, payType)";
    sql.prepare(buildingIndex);
    if(!sql.exec())
    {
        qDebug() << "Error: Fail to BUILDING INDEX_4." << sql.lastError();
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
    double Time2 = time->restart()/1000.0;
    emit(fileloadingFinished(Time1, Time2));

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
void MainWindow::onLoadingFinished(double time1, double time2)
{
    QString status = "Loading files finished in " + QString::number(time1) + "s " +
                     "  Building indexes finished in " + QString::number(time2) + "s";

    for(int i  = 0; i < file_chosen_name_list.count(); i++)
    {

    }
    ui->statusbar->showMessage(status);
    ui->progressBar->hide();
    ui->pushButton_3->setEnabled(true);
    ui->pushButton_6->setEnabled(true);
    ui->comboBox->clear();
    ui->comboBox->addItem("Line A");
    ui->comboBox->addItem("Line B");
    ui->comboBox->addItem("Line C");
    QRegExp range("[1-7][0-9]|80|[0-9]");
    ui->lineEdit_3->setValidator(new QRegExpValidator(range, this));
    /*QIntValidator* IntValidator = new QIntValidator;
    IntValidator->setRange(0, 80);
    ui->lineEdit_3->setValidator(IntValidator);
    //ui->lineEdit_3->setValidator(new QIntValidator(0, 80, this));*/
    qDebug() << "status Finished!  & Progressbar hided!";
    QTreeWidgetItemIterator iterator(ui->treeWidget);
    ui->date->clear();
    days_chosen_list.clear();
    while(*iterator)
    {
        if(
              (
                    ((*iterator)->text(0) == "2019-01-13")
                 || ((*iterator)->text(0) == "2019-01-12")
                 || ((*iterator)->text(0) == "2019-01-11")
                 || ((*iterator)->text(0) == "2019-01-10")
                 || ((*iterator)->text(0) == "2019-01-09")
                 || ((*iterator)->text(0) == "2019-01-08")
                 || ((*iterator)->text(0) == "2019-01-07")

               ) &&
               (
                ((*iterator)->checkState(0) == Qt::Checked) || ((*iterator)->checkState(0) == Qt::PartiallyChecked)
               )
          )
        {
            days_chosen_list << ((*iterator)->text(0));
        }
        ++iterator;
    }
    for (int i = 0; i < days_chosen_list.count(); i++)
        ui->date->addItem(days_chosen_list.at(i));
    ui->Type_of_analyze->clear();
    ui->Type_of_analyze->addItem("Inflow & Outflow");
    if(filter_payType) ui->Type_of_analyze->addItem("PayType Composition");
    if(filter_lineID)
    {
         ui->Type_of_analyze->addItem("Inflow in Different Line");
         ui->Type_of_analyze->addItem("Outflow in Different Line");
    }
    //emit(pushbutton3());
    //ui->tabWidget->setCurrentWidget(ui->tab_SQL);
}
//SQL tab 中的run button
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
    ///在statusbar上显示代码运行结果
    ui->statusbar->showMessage(model->lastError().databaseText(), 100000);
    ui->tableView->setModel(model);
    ui->tableView->show();
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
    ui->pushButton_4->setDisabled(true);
}
//Get the Plan
void MainWindow::on_pushButton_5_clicked()
{
    //读取出入站点
    QString des = ui->lineEdit_2->text();
    QString home = ui->lineEdit->text();
    int des_ID = des.toInt();
    int home_ID = home.toInt();
    if(home_ID > 80 || des_ID > 80)
    {
        QMessageBox::critical(this, "Ooops", "0 <= stationID <= 80");
        return;
    }
    int distance[81];
    for (int i=0; i<81; ++i) distance[i]=-1;
    int prev[81] = {0};
    distance[home_ID] = 0;
    QQueue<int> q;
    q.enqueue(home_ID);
    while (!q.empty())
    {
        int current = q.dequeue();
        if (current == des_ID) break;
        for(int i=0; i<81; ++i)
        {
            if(matrix[current][i]==1 && distance[i]==-1)
            {
                distance[i] = distance[current]+1;
                prev[i] = current;
                q.enqueue(i);
            }
        }
    }
    //生成最短路径
    int *route = new int[distance[des_ID]+1];
    int current = des_ID;
    int i = distance[des_ID];
    while(i>0)
    {
        route[i] = current;
        current = prev[current];
        --i;
    }
    route[0] = current;

    QString Route;
    Route += QString::number(home_ID);
    for (int i=1; i<=distance[des_ID]; ++i)
    {
       Route += " -> ";
       Route += QString::number(route[i]);
    }

    ui->textEdit_2->setText(Route);
    delete [] route;

}
//plot!
void MainWindow::on_pushButton_6_clicked()
{
    if((ui->Type_of_analyze->currentText() == "Inflow & Outflow" ||
        ui->Type_of_analyze->currentText() == "Inflow in Different Line" ||
        ui->Type_of_analyze->currentText() == "Outflow in Different Line") &&
        (ui->mins_edit->text() == "" || ui->hours_edit->text() == ""))
    {
        QMessageBox::information(this, "Ooops", "Please type in \"time step\"");
        return;
    }
    // mins == 0 && hours == 0
    if((ui->Type_of_analyze->currentText() == "Inflow & Outflow" ||
        ui->Type_of_analyze->currentText() == "Inflow in Different Line" ||
        ui->Type_of_analyze->currentText() == "Outflow in Different Line") &&
        ui->mins_edit->text() == "0" &&
        ui->hours_edit->text() =="0")
    {
        QMessageBox::information(this, "Ooops", "Time Step can't be 0");
        return;
    }
    ui->pushButton_6->setEnabled(false);
    bool allStation = false;
    bool allLineID = false;
    int stationID = ui->lineEdit_3->text().toInt();
    QString Type_of_analyze = ui->Type_of_analyze->currentText();
    QString date = ui->date->currentText();
    QString start_time = ui->start_time_edit->text();
    QString end_time =  ui->end_time_edit->text();
    QString start_date_time = date + " " + start_time + ":00";
    //qDebug() <<"start_date_time =" << start_date_time;
    QString end_date_time = date + " " + end_time + ":59";
    //qDebug() <<"end_date_time =" << end_date_time;
    QDateTime start_tmp = QDateTime::fromString(start_date_time, "yyyy-MM-dd hh:mm:ss");
    //qDebug() << "start_tmp = " << start_tmp;
    QDateTime end_tmp = QDateTime::fromString(end_date_time, "yyyy-MM-dd hh:mm:ss");
    //qDebug() << "end_tmp = " << end_tmp;
    auto start_timestamp = start_tmp.toUTC().toSecsSinceEpoch();
    //qDebug() << "start_timestamp = " << start_timestamp;
    auto end_timestamp = end_tmp.toUTC().toSecsSinceEpoch();
    //qDebug() << "end_timestamp = " << end_timestamp;

    if(start_timestamp >= end_timestamp)
    {
        QMessageBox::warning(this, "waring", "Erro: Starting time > Ending time !");
        return;
    }

    int hours = ui->hours_edit->text().toInt();
    int mins = ui->mins_edit->text().toInt();
    if(mins > 60)
    {
        QMessageBox::warning(this, "waring", "Erro: mins > 60 !");
        return;
    }
    int interval = 3600 * hours + 60 * mins;
    if(end_timestamp - start_timestamp <= interval)
    {
        QMessageBox::warning(this, "waring", "Erro: time step > (Ending time - Starting time) !!");
        return;
    }
    if(ui->checkBox->checkState() == Qt::Checked)
    {
        allStation = true;
    }
    if(ui->checkBox_2->checkState() == Qt::Checked)
    {
        allLineID = true;
    }

    QString lineID_chosen;
    if(filter_lineID)
    {
        if(ui->comboBox->currentText() == "Line A")
            lineID_chosen = "A";
        if(ui->comboBox->currentText() == "Line B")
            lineID_chosen = "B";
        if(ui->comboBox->currentText() == "Line C")
            lineID_chosen = "C";
    }

    //analyzing on "Inflow & Outflow"
    auto lambda1 = [=] () -> void
    {
        /*
            qDebug() << start_timestamp;
            qDebug() << end_timestamp;
            qDebug() << stationID;
            qDebug() << interval;
            qDebug() << "worker thread : " << QThread::currentThreadId();
            */
            ///连接之前建立的数据库
            if (QSqlDatabase::contains("lambda1_connection"))
                {
                    database = QSqlDatabase::database("lambda1_connection");
                    qDebug() << "(QSqlDatabase::contains(lambda1_connection))";
                }
            else
            {
                database = QSqlDatabase::addDatabase("QSQLITE", "lambda1_connection");
                database.setConnectOptions("QSQLITE_OPEN_URI;QSQLITE_ENABLE_SHARED_CACHE");
                database.setDatabaseName("file::memory:");
            }
            if (!database.open())
            {
                qDebug() << "Error: Failed to connect database in lambda1 thread" << database.lastError();
            }
            else qDebug() << "database opened successfully in lambda1 thread!";
            QSqlQuery sql(database);
            QString query = "select COUNT(*) from METRO_PASSENGERS where timestamp >= :startTime and timestamp <= :endTime ";
            if(!allStation)
                query += "and stationID = :StationID and status = :Status;";
            else query += "and status = :Status;";
            sql.prepare(query);
            if(!allStation) sql.bindValue(":StationID", stationID);
            int numberOfRows = 0;
            for (qint64 i = start_timestamp; i + interval <= end_timestamp; i += interval)
            {
                sql.bindValue(":startTime", i);
                sql.bindValue(":endTime", i + interval);
                //Outflow:
                sql.bindValue(":Status", 0);
                if(!sql.exec()) qDebug() << "Outflow select failed";

                sql.next();
                numberOfRows = sql.value(0).toInt();
                /*qDebug() << "SELECT * FROM METRO_PASSENGERS WHERE timestamp >= " << i << " AND timestamp <= " << i + interval <<
                            " AND stationID = " << stationID << "and status = " << 0 <<";";*/
                //emit(OutFlowpoint(i+interval/2, numberOfRows, true));
                series->append(1000 * i+interval/2, numberOfRows);
                   if(MAX < numberOfRows) MAX = numberOfRows;

                sql.bindValue(":Status", 1);
                if(!sql.exec()) qDebug() << "Outflow select failed";
                numberOfRows = 0;
                /*
                if(sql.last())
                {
                    numberOfRows =  sql.at() + 1;
                    sql.first();
                    sql.previous();
                }*/
                sql.next();
                numberOfRows = sql.value(0).toInt();
                /*qDebug() << "SELECT * FROM METRO_PASSENGERS WHERE timestamp >= " << i << " AND timestamp <= " << i + interval <<
                            " AND stationID = " << stationID << "and status = " << 1 <<";"; */
                //emit(OutFlowpoint(i+interval/2, numberOfRows, false));
                series_in->append(1000 * i+interval/2, numberOfRows);
                   if(MAX < numberOfRows) MAX = numberOfRows;
            }
            emit(plot_finished());
            database.close();
    };
    auto lambda2 = [=] () -> void
    {
        ///连接之前建立的数据库
        if (QSqlDatabase::contains("lambda2_connection"))
            {
                database = QSqlDatabase::database("lambda2_connection");
                qDebug() << "(QSqlDatabase::contains(lambda2_connection))";
            }
        else
        {
            database = QSqlDatabase::addDatabase("QSQLITE", "lambda2_connection");
            database.setConnectOptions("QSQLITE_OPEN_URI;QSQLITE_ENABLE_SHARED_CACHE");
            database.setDatabaseName("file::memory:");
        }
        if (!database.open())
        {
            qDebug() << "Error: Failed to connect database in lambda2 thread" << database.lastError();
        }
        else qDebug() << "database opened successfully in lambda2 thread!";
        QSqlQuery sql(database);
        QString query = "select COUNT(*) from METRO_PASSENGERS where timestamp >= :startTime and timestamp <= :endTime ";
        if(!allStation)
            query += "and stationID = :StationID and status = 1 and payType = :PayType;";
        else query += "and status = 1 and payType = :PayType;";
        sql.prepare(query);
        if(!allStation) sql.bindValue(":StationID", stationID);
        int numberOfRows_type0 = 0;
        int numberOfRows_type1 = 0;
        int numberOfRows_type2 = 0;
        int numberOfRows_type3 = 0;
        sql.bindValue(":startTime", start_timestamp);
        sql.bindValue(":endTime", end_timestamp);

        sql.bindValue(":PayType", 0);
        if(!sql.exec()) qDebug() << "Outflow select failed";
        numberOfRows_type0 = 0;
        sql.next();
        numberOfRows_type0 = sql.value(0).toInt();
        /*qDebug() << "SELECT * FROM METRO_PASSENGERS WHERE timestamp >= " << start_time << " AND timestamp <= " << i + interval <<
                        " AND stationID = " << stationID << "and status = " << 1 <<";"; */

        sql.bindValue(":PayType", 1);
        if(!sql.exec()) qDebug() << "Outflow select failed";
        numberOfRows_type1 = 0;
        sql.next();
        numberOfRows_type1 = sql.value(0).toInt();

        sql.bindValue(":PayType", 2);
        if(!sql.exec()) qDebug() << "Outflow select failed";
        numberOfRows_type2 = 0;
        sql.next();
        numberOfRows_type2 = sql.value(0).toInt();

        sql.bindValue(":PayType", 3);
        if(!sql.exec()) qDebug() << "Outflow select failed";
        numberOfRows_type3 = 0;
        sql.next();
        numberOfRows_type3 = sql.value(0).toInt();

        emit(payType_plot_finished(numberOfRows_type0, numberOfRows_type1, numberOfRows_type2, numberOfRows_type3));
        database.close();
    };
    auto lambda3 = [=] () -> void
    {
        ///连接之前建立的数据库
        if (QSqlDatabase::contains("lambda3_connection"))
            {
                database = QSqlDatabase::database("lambda3_connection");
                qDebug() << "(QSqlDatabase::contains(lambda3_connection))";
            }
        else
        {
            database = QSqlDatabase::addDatabase("QSQLITE", "lambda3_connection");
            database.setConnectOptions("QSQLITE_OPEN_URI;QSQLITE_ENABLE_SHARED_CACHE");
            database.setDatabaseName("file::memory:");
        }
        if (!database.open())
        {
            qDebug() << "Error: Failed to connect database in lambda3 thread" << database.lastError();
        }
        else qDebug() << "database opened successfully in lambda3 thread!";
        QString query = "select COUNT(*) from METRO_PASSENGERS where timestamp >= :startTime and timestamp <= :endTime ";
        QSqlQuery sql(database);
        if(!allStation)
        {
            if(!allLineID)
            {
                query += "and stationID = :StationID and status = :Status and lineID = :LineID;";
                sql.prepare(query);
                sql.bindValue(":StationID", stationID);
                sql.bindValue(":LineID", lineID_chosen);
            }
            else
            {
                query += "and stationID = :StationID and status = :Status";
                sql.prepare(query);
                sql.bindValue(":StationID", lineID_chosen);
            }
        }
        else
        {
            query += "and status = :Status and lineID = :LineID;";
            sql.prepare(query);
            sql.bindValue(":LineID", lineID_chosen);
        }
        int numberOfRows = 0;
        for (qint64 i = start_timestamp; i + interval <= end_timestamp; i += interval)
        {
            sql.bindValue(":startTime", i);
            sql.bindValue(":endTime", i + interval);
            //Outflow:
            sql.bindValue(":Status", 0);
            if(!sql.exec()) qDebug() << "Outflow select failed";

            sql.next();
            numberOfRows = sql.value(0).toInt();
            /*qDebug() << "SELECT * FROM METRO_PASSENGERS WHERE timestamp >= " << i << " AND timestamp <= " << i + interval <<
                        " AND stationID = " << stationID << "and status = " << 0 <<";";*/
            //emit(OutFlowpoint_diffline(i+interval/2, numberOfRows, true));
            series_diffline->append(1000 * i + interval/2,  numberOfRows);
            if(MAX < numberOfRows) MAX = numberOfRows;
        }
        emit(OutFlow_diffline_finished(true));
        database.close();
    };
    auto lambda4 = [=] () -> void
    {
        ///连接之前建立的数据库
        if (QSqlDatabase::contains("lambda4_connection"))
            {
                database = QSqlDatabase::database("lambda4_connection");
                qDebug() << "(QSqlDatabase::contains(lambda4_connection))";
            }
        else
        {
            database = QSqlDatabase::addDatabase("QSQLITE", "lambda4_connection");
            database.setConnectOptions("QSQLITE_OPEN_URI;QSQLITE_ENABLE_SHARED_CACHE");
            database.setDatabaseName("file::memory:");
        }
        if (!database.open())
        {
            qDebug() << "Error: Failed to connect database in lambda4 thread" << database.lastError();
        }
        else qDebug() << "database opened successfully in lambda4 thread!";
        QString query = "select COUNT(*) from METRO_PASSENGERS where timestamp >= :startTime and timestamp <= :endTime ";
        QSqlQuery sql(database);
        if(!allStation)
        {
            if(!allLineID)
            {
                query += "and stationID = :StationID and status = :Status and lineID = :LineID;";
                sql.prepare(query);
                sql.bindValue(":StationID", stationID);
                sql.bindValue(":LineID", lineID_chosen);
            }
            else
            {
                query += "and stationID = :StationID and status = :Status";
                sql.prepare(query);
                sql.bindValue(":StationID", stationID);
            }
        }
        else
        {
            query += "and status = :Status and lineID = :LineID;";
            sql.prepare(query);
            sql.bindValue(":LineID", lineID_chosen);
        }
        int numberOfRows = 0;
        for (qint64 i = start_timestamp; i + interval <= end_timestamp; i += interval)
        {
            sql.bindValue(":startTime", i);
            sql.bindValue(":endTime", i + interval);
            //Inflow:
            sql.bindValue(":Status", 1);
            //qDebug()   << query;
            if(!sql.exec()) qDebug() << "Inflow select failed";

            sql.next();
            numberOfRows = sql.value(0).toInt();
            /*qDebug() << query;
            qDebug() << "SELECT * FROM METRO_PASSENGERS WHERE timestamp >= " << i << " AND timestamp <= " << i + interval <<
                        " AND stationID = " << stationID << "and status = " << 1 <<"and lineID = " << lineID_chosen <<";";
            qDebug() << "(" << i + interval/2 << ", " << numberOfRows << ")";
            */
            //emit(OutFlowpoint_diffline(i+interval/2, numberOfRows, false));
            series_diffline_in->append(1000 * i + interval/2,  numberOfRows);
            if(MAX < numberOfRows) MAX = numberOfRows;
        }
        emit(OutFlow_diffline_finished(false));
        database.close();
    };
    if(Type_of_analyze == "Inflow & Outflow")
    {
         chart = new QChart();
         QString title;
         if(!allStation)
            title = "Inflow & Outflow of Station " + QString::number(stationID) + " from " + start_date_time + " to " + end_date_time;
         else title = "Inflow & Outflow of All Stations from " + start_date_time + " to " + end_date_time;
         chart->setTitle(title);
         series = new QSplineSeries();
         series_in = new QSplineSeries();
         QFuture<void> future = QtConcurrent::run(lambda1);
    }
    if(Type_of_analyze == "PayType Composition")
    {
         chart = new QChart();
         QString title;
         if(!allStation)
              title = "PayType Composition of Inflow in Station " + QString::number(stationID) + " from " + start_date_time + " to " + end_date_time;
         else title = "PayType Composition of Inflow in All Stations from " + start_date_time + " to " + end_date_time;
         chart->setTitle(title);
         payType_series = new QPieSeries();
         QFuture<void> future = QtConcurrent::run(lambda2);
    }
    if(Type_of_analyze == "Outflow in Different Line")
    {
         chart = new QChart();
         QString title;
         if(!allStation)
         {
              if(!allLineID)
                title = "Outflow of Line " + lineID_chosen + " in Station " + QString::number(stationID) + " from " + start_date_time + " to " + end_date_time;
              else title = "Outflow of all lines " + lineID_chosen + " in Station " + QString::number(stationID) + " from " + start_date_time + " to " + end_date_time;
         }
         else
         {
             if(!allLineID)
               title = "Outflow of Line " + lineID_chosen + " in all stations from " + start_date_time + " to " + end_date_time;
             else title = "Outflow of all lines " + lineID_chosen + " in all stations from " + start_date_time + " to " + end_date_time;
         }
         chart->setTitle(title);
         series_diffline = new QSplineSeries();
         QFuture<void> future = QtConcurrent::run(lambda3);
    }
    if(Type_of_analyze == "Inflow in Different Line")
    {
         chart = new QChart();
         QString title;
         if(!allStation)
         {
              if(!allLineID)
                title = "Inflow of Line " + lineID_chosen + " in Station " + QString::number(stationID) + " from " + start_date_time + " to " + end_date_time;
              else title = "Inflow of all lines " + lineID_chosen + " in Station " + QString::number(stationID) + " from " + start_date_time + " to " + end_date_time;
         }
         else
         {
             if(!allLineID)
               title = "Inflow of Line " + lineID_chosen + " in all stations from " + start_date_time + " to " + end_date_time;
             else title = "Inflow of all lines " + lineID_chosen + " in all stations from " + start_date_time + " to " + end_date_time;
         }
         chart->setTitle(title);
         series_diffline_in = new QSplineSeries();
         QFuture<void> future = QtConcurrent::run(lambda4);
    }

}

/*
void MainWindow::on_OutFlowpoint_diffline(long long x, long long y, bool isOUT)
{
    if(isOUT)
    {
        //qDebug() << "OUT";
        series_diffline->append(1000 * x, y);
        if(MAX < y) MAX = y;
        //qDebug() <<"--->OUT";
    }
    else
    {
        //qDebug() << "IN";
        series_diffline_in->append(1000 * x, y);
           if(MAX < y) MAX = y;
        //qDebug() << "--->IN";
    }
}
*/
void MainWindow::on_OutFlow_diffline_finished(bool isOUT)
{
    if(isOUT)
    {
        //QPen pen1(Qt::blue, 8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        series_diffline->setPointsVisible();
        series_diffline->setName("Outflow");
        QFont font;
        font.setPixelSize(20);
        chart->setTitleFont(font);
        chart->setTitleBrush(QBrush(Qt::black));
        chart->addSeries(series_diffline);
        auto Y = new QValueAxis(chart);
        Y->setLabelFormat("%d");
        Y->setMax(1.1 * MAX);
        chart->addAxis(Y, Qt::AlignLeft);
        series_diffline->attachAxis(Y);
        auto X = new QDateTimeAxis(chart);
        X->setFormat("hh:mm");
        chart->addAxis(X, Qt::AlignBottom);
        series_diffline->attachAxis(X);
    }
    else
    {
        //QPen pen2(Qt::darkRed, 8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        series_diffline_in->setPointsVisible();
        series_diffline_in->setName("Inflow");
        QFont font;
        font.setPixelSize(20);
        chart->setTitleFont(font);
        chart->setTitleBrush(QBrush(Qt::black));
        chart->addSeries(series_diffline_in);
        auto Y = new QValueAxis(chart);
        Y->setLabelFormat("%d");
        Y->setMax(1.1 * MAX);
        chart->addAxis(Y, Qt::AlignLeft);
        series_diffline_in->attachAxis(Y);
        auto X = new QDateTimeAxis(chart);
        X->setFormat("hh:mm");
        chart->addAxis(X, Qt::AlignBottom);
        series_diffline_in->attachAxis(X);
    }

    ui->ChartView->setRenderHint(QPainter::Antialiasing);
    ui->ChartView->setChart(chart);
    MAX = 0;
    ui->pushButton_6->setEnabled(true);
}
void MainWindow::on_building_index_of_sql(double time)
{
    QString message;
    message = "File loading finished in " +  QString::number(time) + "s ----> Building the index of sql now...";
    ui->statusbar->showMessage(message);
}
void MainWindow::on_plot_finished()
{
    QPen pen1(Qt::blue, 8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen pen2(Qt::darkRed, 8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    series->setPointsVisible();
    series->setName("Outflow");
    series->setPen(pen2);
    series_in->setPointsVisible();
    series_in->setName("Inflow");
    series_in->setPen(pen1);
    QFont font;
    font.setPixelSize(20);
    chart->setTitleFont(font);
    chart->setTitleBrush(QBrush(Qt::black));
    chart->addSeries(series);
    chart->addSeries(series_in);
    auto Y = new QValueAxis(chart);
    Y->setLabelFormat("%d");
    Y->setMax(1.1 * MAX);
    chart->addAxis(Y, Qt::AlignLeft);
    series->attachAxis(Y);
    series_in->attachAxis(Y);
    auto X = new QDateTimeAxis(chart);
    X->setFormat("hh:mm");
    chart->addAxis(X, Qt::AlignBottom);
    series->attachAxis(X);
    series_in->attachAxis(X);
    ui->ChartView->setRenderHint(QPainter::Antialiasing);
    ui->ChartView->setChart(chart);
    ui->pushButton_6->setEnabled(true);
    MAX = 0;
}
void MainWindow::on_checkBox_stateChanged(int arg1)
{
    if(ui->checkBox->checkState() != Qt::Checked)
          ui->lineEdit_3->setEnabled(true);
    if(ui->checkBox->checkState() != Qt::Unchecked)
    {
          ui->lineEdit_3->setEnabled(false);
          ui->lineEdit_3->clear();
    }

}
void MainWindow::on_payType_plot_finished(int t0, int t1, int t2, int t3)
{
    double sum = t0 + t1 + t2 + t3;
    QString type0 = "payType 0 -> " + QString::number(t0 / sum * 100) + "%";
    QString type1 = "payType 1 -> " + QString::number(t1 / sum * 100) + "%";
    QString type2 = "payType 2 -> " + QString::number(t2 / sum * 100) + "%";
    QString type3 = "payType 3 -> " + QString::number(t3 / sum * 100) + "%";
    qDebug() << type0;
    qDebug() << type1;
    qDebug() << type2;
    qDebug() << type3;
    payType_series->append(type0, double(t0 / sum * 10));
    payType_series->append(type1, double(t1 / sum * 10));
    payType_series->append(type2, double(t2 / sum * 10));
    payType_series->append(type3, double(t3 / sum * 10));
    payType_series->setLabelsVisible();
    chart->addSeries(payType_series);
    ui->ChartView->setRenderHint(QPainter::Antialiasing);
    ui->ChartView->setChart(chart);
    ui->pushButton_6->setEnabled(true);
}
void MainWindow::on_Type_of_analyze_activated(const QString &arg1)
{
    if(ui->Type_of_analyze->currentText() == "PayType Composition")
    {
        ui->hours_edit->clear();
        ui->mins_edit->clear();
        ui->hours_edit->setDisabled(true);
        ui->mins_edit->setDisabled(true);
        ui->comboBox->setDisabled(true);
        ui->checkBox_2->setDisabled(true);
    }
    if(ui->Type_of_analyze->currentText() == "Inflow & Outflow")
    {
        ui->hours_edit->setEnabled(true);
        ui->mins_edit->setEnabled(true);
        ui->comboBox->setDisabled(true);
        ui->checkBox_2->setDisabled(true);
    }
    if(ui->Type_of_analyze->currentText() == "Inflow in Different Line" ||
       ui->Type_of_analyze->currentText() == "Outflow in Different Line")
    {
        ui->hours_edit->setEnabled(true);
        ui->mins_edit->setEnabled(true);
        ui->comboBox->setEnabled(true);
        ui->checkBox_2->setEnabled(true);
        ui->lineEdit_3->setText("76");
    }
}
void MainWindow::on_checkBox_2_stateChanged(int arg1)
{
    if(ui->checkBox_2->checkState() != Qt::Checked)
          ui->comboBox->setEnabled(true);
    if(ui->checkBox_2->checkState() != Qt::Unchecked)
    {
          ui->comboBox->setEnabled(false);
    }
}
