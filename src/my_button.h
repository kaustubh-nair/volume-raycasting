
#pragma once
#include <QPushButton>

class MyQPushButton : public QPushButton
{
Q_OBJECT
    public:
        MyQPushButton(QString text, QWidget *parent = nullptr)
            :QPushButton(text, parent)
        {}
        virtual ~MyQPushButton(){}

signals:
        void myClickedWithName(QString name);
public slots:
    void myClicked()
    {
        emit myClickedWithName(this->objectName());
    }

};
