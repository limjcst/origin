#include "func.h"

void setLabel(QLabel* label)
{
	QPalette labelP;
	labelP.setColor(QPalette::Window, QColor(255, 255, 255, 100));
	label->setAutoFillBackground(true);
	//set the background of label to white
	label->setPalette(labelP);
	label->setFont(QFont("serif", 10));
}
