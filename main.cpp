#include "fir.h"
#include <QApplication>
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	FIR w;
	w.show();
	return a.exec();
}
