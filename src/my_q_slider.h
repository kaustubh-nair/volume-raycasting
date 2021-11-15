#pragma once
#include <QSlider>

class MyQSlider : public QSlider
{
Q_OBJECT
    public:
        MyQSlider(Qt::Orientation orientation, QWidget *parent = nullptr)
            :QSlider(orientation, parent)
        {}
        virtual ~MyQSlider(){}

signals:
        void myValueChangedWithId(int value, QString name);
public slots:
    void myValueChanged(int value)
    {
        printf("%s\n", this->objectName().toStdString().c_str());
        emit myValueChangedWithId(value, this->objectName());
    }

};
