#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QString sPath = "C:/";
    dirmodel = new QFileSystemModel(this);
    dirmodel->setRootPath(sPath);
    ui->treeView->setModel(dirmodel);
    QModelIndex index = dirmodel->index("C:/");
    ui->treeView->expand(index);
    ui->treeView->scrollTo(index);
    ui->treeView->setAnimated(true); //开启折叠动画
    ui->treeView->setSortingEnabled(true); //开启sorting权限
    ui->treeView->resizeColumnToContents(0); //保证第一列够宽

//    ui->treeWidget->setColumnCount(2);
//    ui->treeWidget->setHeaderLabels(QStringList() << "one" << "two");
//    AddRoot("hello", "world");
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    //load the file
    QString file_name = QFileDialog::getOpenFileName(this, "Please choose the data set folder", QDir::homePath());

}

//void MainWindow::AddRoot(QString name, QString Description)
//{
//    QTreeWidgetItem *itm = new QTreeWidgetItem(ui->treeWidget);
//    itm->setText(0, name);
//    itm->setText(1, Description);

//    AddChild(itm, "one", "hello");
//    AddChild(itm, "two", "hello");
//}
//void MainWindow::AddChild(QTreeWidgetItem *parent,QString name, QString Description)
//{
//    QTreeWidgetItem *itm = new QTreeWidgetItem();
//    itm->setText(0, name);
//    itm->setText(1, Description);
//    parent->addChild(itm);
//}

//void MainWindow::on_pushButton_2_clicked()
//{
//    //make dir
//    QModelIndex index = ui->treeView->currentIndex();
//    if(!index.isValid()) return;

//    QString name = QInputDialog::getText(this, "Name", "Enter a name");
//    if(name.isEmpty()) return;

//    model->mkdir(index, name);

//}

//void MainWindow::on_pushButton_3_clicked()
//{
//    //delete dir
//    QModelIndex index = ui->treeView->currentIndex();
//    if(!index.isValid()) return;

//    if(model->fileInfo(index).isDir())
//    {
//        //dir
//        model->rmdir(index);
//    }
//    else
//    {
//        //file
//        model->remove(index);
//    }
//}
