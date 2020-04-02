# 🚉 Hangzhou Metro Data Visualization
***Environment: Windows 10***---
***Inplemented with Qt Creator***

## Introduction

----

![image-20200402111627661](https://i.loli.net/2020/04/02/k6QAJ9nwyDP48jS.png)

**"Metro Insider"*** handles files with **SQLite in-memory database** with high efficiency,  the following modules are implemented:

-> Loading Files with a Selector and Filter

-> SQLite Runner

-> Plotting & Analyzing

-> Route Planning

## Functions & Implementations Details

----

### Module 1 *-> Loading Files with a Selector and Filters*

#### Select Files and Set Filters

"choose a folder" button is expected to be pushed to start the whole process. It will open a dialogue to let users to **select the "dataset" folder**. It is implemented with `QFileDialog::getExistingDirectory()`.

![image-20200402110928691](https://i.loli.net/2020/04/02/6DShV9b3yzJ7pgA.png)

Then a tree widget is created to let users choose files and filters implemented with `QTreeWidegetsItem`.

##### -> Files Tree

**The files tree can synchronize the child node with the parent node all the time**, with `Qt::Checked; Qt::PartlyChecked; Qt:Unchecked;` which means that 

- parent `Partlychecked` $\Leftrightarrow$ some children `Checked`
- parent `Checked`  $\Leftrightarrow$ all children `Checked `
- parent `Unchecked` $\Leftrightarrow$ all children `Unchecked`
- *It's implemented recursively.*

And names of all the chosen files is maintained in `QStringList chosen_files_names` timely **before**

the "load chosen files" bottom being pushed. (So the program won't have any problems when users change the checked items during the loading process.)

##### -> Filters Tree

It is designed for choosing the fields to be loaded into the **SQLite** database. But "time" & "stationID" & "status" can **not** be unchecked since our mandatory task requires a plot on "Inflow & Outflow of a station".

#### Loading Selected Files 

Only selected fields of selected files will be loaded into  the datatbase. The database would create a table whose columns are those fields. And this module has the following characteristics:

##### **-> High  efficiency**

Loading the dataset of one day takes averagely  50s in my surface-Laptop (2017, 13.5'), it's relatively fast due to the insertion of data to SQLite can't be done by multicore.

##### **-> Multithreading**

With `<QtConcurrent>` , the loading process is being done in another thread so the ui-thread will never be frozen.

##### **-> Status Bar**

The status bar is well designed to respond quickly to any situation

![image-20191228185428567](https://i.loli.net/2020/04/02/47sSipIzyUtXjOv.png)

![image-20191228185746754](https://i.loli.net/2020/04/02/WdPwh2aovfGTpRQ.png)

![image-20191228185843719](https://i.loli.net/2020/04/02/p5dFbqVU8ZDMlE1.png)

If users change filters or files selections after a loading, the status bar will suggest a renew.

![image-20191228190716316](https://i.loli.net/2020/04/02/s5VmPJHtchfRQei.png) 

##### **-> Robustness**

- Loading process is based on a file list maintained **before** "pressing the load button", so even if users change the file selection box or filters during the load, they won't have a problem.
- Reloading is well supported since the database will drop and create a new table newly designed for the chosen column and ensure there will not be a leak of memory.
- Before the loading process have been done, all the buttons and combo boxes relied to chosen files and filters will be disabled to avoid possible errors.

---

### Module 2 *-> SQLite Runner*

![img](https://i.loli.net/2020/04/02/QxjoAkR7mtwFVqT.png)
*SQLite Runner* is designed for any explore of raw data by users. It is implemented by `QTextEdit` `QSqlQueryModel` `QTableView` , it has following characteristics:

##### **-> Fast Speed**

- The database is in-memory and has a carefully designed **composite index**. So almost all the `SELECT` operation can be done in 0.x seconds.
- `QTableView` with `QSqlQuerymodel`makes it possible for **dynamic loading**, which means that the ui will not show all the results from e.g. `SELECT * FROM TABLE` at once so the ui will not freeze at any time. When users scrolls the table view, new query results will constantly be loaded

##### **-> Syntax Error & Running Error Information**

This function is implemented by 

`ui->statusbar->showMessage(model->lastError().databaseText(), 100000);`

It will show any error message in the status bar.

---

### Module3 -> Plotting & Analyzing

![1585796701251](https://i.loli.net/2020/04/02/oGhCH2MexNWLZKi.jpg)

![1585796708357](https://i.loli.net/2020/04/02/9GSg1oCmdzKtB3V.jpg)

![1585796715361](https://i.loli.net/2020/04/02/FYUP4sbg5cLKB8y.jpg)

