/* worker1 is responsible for loading and processing the chose files*/

#ifndef WORKER1_H
#define WORKER1_H

#include <QObject>

class worker1 : public QObject
{
    Q_OBJECT
public:
    explicit worker1(QObject *parent = nullptr);
    void run();
signals:

public slots:
};

#endif // WORKER1_H
