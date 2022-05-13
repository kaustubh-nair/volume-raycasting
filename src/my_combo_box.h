#pragma once
#include <QComboBox>

class MyQComboBox : public QComboBox
{
Q_OBJECT
    public:
        MyQComboBox(QWidget *parent = nullptr)
            :QComboBox(parent)
        {}
        virtual ~MyQComboBox(){}

signals:
        void myCurrentIndexChangedWithId(int value, QString name);
public slots:
    void myCurrentIndexChanged(int value)
    {
        emit myCurrentIndexChangedWithId(value, this->objectName());
    }

};
