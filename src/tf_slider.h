#include <QSlider>

class TFSlider : public QSlider
{
	Q_OBJECT
    TFSlider(Qt::Orientation orientation, QWidget *parent = nullptr);

	public signals:
		void valueChanged(int value){}


	public slots:
		void setValue(int value){}
		void setMinimum(int value){}
};
