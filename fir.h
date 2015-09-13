#ifndef FIR_H
#define FIR_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QHostInfo>
#include <QList>
#include <QTimer>
#include <QTime>
#include <QVector>
#include <QAudioOutput>
#include <QFile>
#include <QAudioFormat>
#include <QStringListModel>
#include <QMessageBox>

namespace Ui {
class FIR;
}

const int scale = 15;
const int tScale = scale * scale;
const int len = 36;
const int space = 10;
const int interval = 20;

class FIR : public QWidget
{
	Q_OBJECT
	typedef void (FIR::*fun)();
public:
	explicit FIR(QWidget *parent = 0);
	~FIR();
signals:
	void readyDecode(QString &);
	void readyCode(int mode, int x = 0, int y = 0);
private slots:
	void on_pushButton_5_clicked();

	void on_pushButton_7_clicked();

	void on_pushButton_9_clicked();

	void on_pushButton_2_clicked();

	bool on_Withdraw_clicked();

	void on_selectOk_clicked();

	void on_Save_clicked();
private slots:
	void Accept();
	void Receive();
	void BreakConnect();
	void TimeOut();
	void Decode(QString &);
	void Code(int mode, int x = 0, int y = 0);
	void conFinished();
	void quitFinished();
	void loseFinished();
	void withFinished();
protected:
	void paintEvent(QPaintEvent *);
	void mousePressEvent(QMouseEvent *);
	void closeEvent(QCloseEvent *);
private:
	bool getPoint(int &, int &);
	bool isWin(int x, int y);
	bool isLegalPoint(int, int);
	void turnChange();
	void Start();
	void Stop();
	void Win();
	void Lose();
	void Withdraw(int t_color);
	void Load();
	void setMBox(QMessageBox *MBox, fun, QString str, QString sub = "Ignore");
	bool turn;
	bool start;
	bool startHaveAsk;
	bool startAsked;
	bool breakHaveAsk;
	bool end;
	int color;
	int localTime, peerTime;
	int restTime;
	Ui::FIR *ui;
	QString ip;
	QString hostip;
	int port;
	int hostport;
	int cols[scale + 2], rows[scale + 2];
	int map[scale][scale];
	bool listen;
	bool connected;
	QList<QHostAddress> list;
	QTcpServer* server;
	QTcpSocket* socket;
	QTimer timer, time;
	QTime zero;
	QVector<QPoint> put;
	int withdrawNum;
	QVector<int *> bak;
	int temp[tScale + 1];
	QFile* soundWin;
	QAudioOutput *audioWin;
	QFile* soundLose;
	QAudioOutput *audioLose;
	QAudioFormat format;
	QStringList strList;
	QStringListModel *model;
	QMessageBox *withdrawMBox;
	QMessageBox *loseMBox;
	QMessageBox *quitMBox;
	QMessageBox *connectMBox;
};

#endif // FIR_H
