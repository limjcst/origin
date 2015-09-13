#include "fir.h"
#include "ui_fir.h"
#include "func.h"

#include <QDebug>
#include <QRegExp>
#include <QRegExpValidator>
#include <QPainter>
#include <QMouseEvent>
#include <QFile>
#include <QStringBuilder>
#include <QTextStream>

//1 white;2 black

FIR::FIR(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::FIR)
{
	ui->setupUi(this);
	this->setFixedSize(1000, 600);
	server = new QTcpServer(this);
	QHostInfo info(QHostInfo::fromName(QHostInfo::localHostName()));
	list = info.addresses();
	QString local = "127.0.0.1";
	list.push_back(QHostAddress(local));
	list.push_back(QHostAddress::Any);
	QHostAddress addr;
	for (int i = 0; i < list.length() - 2; ++i)
	{
		if (list[i].protocol() == QAbstractSocket::IPv4Protocol)
			addr = list[i];
		ui->comboBox->addItem(list[i].toString());
	}
	ui->comboBox->addItem(local);
	ui->comboBox->addItem("All");
	QRegExpValidator* portValidator = new QRegExpValidator(this);
	portValidator->setRegExp(QRegExp("[0-9]*"));
	ui->settingPort->setValidator(portValidator);
	ip = addr.toString();
	ui->IPAddress->setText(ip);
	ui->comboBox->setCurrentText(ip);
	port = 8888;
	ui->port->setText(QString::number(port));
	ui->settingPort->setText(QString::number(port));
	ui->widget->setHidden(true);
	ui->widget->setParent(NULL);
	server->listen(addr, port);
	listen = true;
	socket = NULL;
	connect(server, SIGNAL(newConnection()),
			this, SLOT(Accept()));

	ui->widget_2->setParent(NULL);
	ui->widget_2->setHidden(true);
	QRegExpValidator* IPValidator = new QRegExpValidator(this);
	IPValidator->setRegExp(QRegExp("[0-9.a-f:]*"));
	ui->HostIP->setValidator(IPValidator);
	ui->HostPort->setValidator(portValidator);
	ui->HostIP->setText(local);
	ui->HostPort->setText(QString::number(port));
	//socket = NULL;
	connect(&timer, SIGNAL(timeout()),
			this, SLOT(BreakConnect()));

	connected = false;
	cols[0] = 0;
	rows[0] = 0;
	for (int i = 1; i <= scale + 1; ++i)
	{
		cols[i] = cols[i - 1] + len;
		rows[i] = cols[i];
	}
	time.setInterval(1000);
	connect(&time, SIGNAL(timeout()),
			this, SLOT(TimeOut()));
	Stop();
	srand(zero.currentTime().msecsSinceStartOfDay());
	for (int i = 0; i < scale; ++i)
		for (int j = 0; j < scale; ++j) map[i][j] = 0;
	//memset(map, 0, sizeof(map));
	zero.setHMS(0, 0, 0);
	ui->widget_3->setHidden(true);
	ui->widget_3->setParent(NULL);
	ui->widget_3->setLayout(ui->horizontalLayout_10);
	//initialize messageboxs
	withdrawMBox = new QMessageBox(this);
	quitMBox = new QMessageBox(this);
	loseMBox = new QMessageBox(this);
	connectMBox = new QMessageBox(this);
	setMBox(withdrawMBox, withFinished, tr("Your peer asks for withdraw."));
	setMBox(loseMBox, loseFinished,tr("Sorry to say that you have lost."), "Withdraw");
	setMBox(quitMBox, quitFinished, tr("Your peer asks for quit."));
	setMBox(connectMBox, conFinished, tr("Some one wants to connect with you."));
	//initialize labels
	setLabel(ui->label);
	setLabel(ui->label_2);
	setLabel(ui->label_7);
	setLabel(ui->label_8);
	setLabel(ui->State);
	setLabel(ui->Color);
	ui->Color->setText("     ");
	Load();
	connect(this, SIGNAL(readyDecode(QString&)),
			this, SLOT(Decode(QString&)));
	connect(this, SIGNAL(readyCode(int,int,int)),
			this, SLOT(Code(int,int,int)));
	//sound
	soundWin = new QFile(":/sound/win");
	soundWin->open(QIODevice::ReadOnly);
	soundLose = new QFile(":/sound/lose");
	soundLose->open(QIODevice::ReadOnly);
	format.setSampleRate(8000);
	format.setChannelCount(2);
	format.setSampleSize(16);
	format.setCodec("audio/pcm");
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setSampleType(QAudioFormat::UnSignedInt);
	audioWin = new QAudioOutput(format, this);
	format.setSampleRate(11025);
	audioLose = new QAudioOutput(format, this);
}

FIR::~FIR()
{
	delete ui;
}
//slots
void FIR::on_pushButton_5_clicked()//set Ok
{
	QHostAddress addr = list[ui->comboBox->currentIndex()];
	ip = addr.toString();
	port = ui->settingPort->text().toInt();
	ui->IPAddress->setText(ip);
	ui->port->setText(QString::number(port));
	if (socket == NULL)
	{
		if (listen)
			server->close();
		server->listen(addr, port);
		listen = true;
	}
	ui->widget->setHidden(true);
}

void FIR::on_pushButton_7_clicked()//inner creat
{
	hostip = ui->HostIP->text();
	hostport = ui->HostPort->text().toInt();
	ui->widget_2->setHidden(true);
	ui->pushButton->setDisabled(true);
	socket = new QTcpSocket(this);
	listen = false;
	server->close();
	socket->connectToHost(hostip, hostport);
	timer.setInterval(20 * 1000);
	//timer.start();
	connect(socket, SIGNAL(readyRead()),
			this, SLOT(Receive()));
}

void FIR::on_pushButton_9_clicked()//break
{
	if (socket != NULL)
	{
		if (start || breakHaveAsk)
		{
			emit readyCode(255);
		}else
		{
			Stop();
			socket->write("#255#");
			connected = false;
			ui->pushButton_4->setDisabled(false);
			timer.setInterval(1000);
			timer.start();
		}
	}
}

void FIR::on_pushButton_2_clicked()//start
{
	if (socket != NULL && connected && !startHaveAsk)
	{
		if (listen)
			ui->widget_3->setHidden(false);
		else
			emit readyCode(2);
	}
}

bool FIR::on_Withdraw_clicked()//Withdraw
{
	bool flag = false;
	foreach (QPoint p, put)
		if (map[p.x()][p.y()] == color)
		{
			flag = true;
			break;
		}
	if (flag && startHaveAsk && withdrawNum < 2 && turn)
	{
		start = false;
		emit readyCode(252);
		return true;
	}
	return false;
}

void FIR::on_selectOk_clicked()
{
	ui->widget_3->setHidden(true);
	emit readyCode(2);
}

void FIR::on_Save_clicked()
{
	if (start)
	{
		QByteArray arr;
		int *array = new int[tScale + 1];
		arr.clear();
		QFile out("bak.bak");
		if (out.open(QIODevice::ReadOnly))
		{
			out.close();
			out.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);
		}else
			out.open(QIODevice::WriteOnly);
		QString builder;
		builder = "@";
		builder = builder % QDate::currentDate().toString(Qt::ISODate);
		builder = builder % " " % QTime::currentTime().toString();
		strList.push_back(builder);
		model->setStringList(strList);
		ui->listView->setModel(model);
		for (int i = 0; i < scale; ++i)
			for (int j = 0; j < scale; ++j)
			{
				builder = builder % " " % QString::number(map[i][j]);
				array[i + j * scale] = map[i][j];
			}
		if (turn)
		{
			builder = builder % " " % QString::number(color);
			array[tScale] = color;
		}else
		{
			builder = builder % " " % QString::number(3 - color);
			array[tScale] = 3 - color;
		}
		bak.push_back(array);
		arr.append(builder);
		out.write(arr);
		out.close();
	}
}

void FIR::Accept()
{
	if (socket == NULL)
	{
		connectMBox->setHidden(false);
		return;
	}
	QTcpSocket *s = server->nextPendingConnection();
	s->write("#255#");
	s->disconnectFromHost();
	delete s;
}

void FIR::Receive()
{
	QString str = socket->readAll();
	QStringList sList = str.split("#");
	for (int i = 0; i < sList.length(); ++i)
	{
		str = sList[i];
		if (str != "")
			emit readyDecode(str);
	}
}

void FIR::BreakConnect()
{
	if (socket != NULL)
	{
		connected = false;
		ui->pushButton_4->setDisabled(false);
		socket->disconnectFromHost();
		delete socket;
		socket = NULL;
		if (!listen)
			on_pushButton_5_clicked();
		ui->pushButton->setDisabled(false);
		ui->textBrowser->append("Failed to connect to the host.");
	}
	if (timer.isActive())
		timer.stop();
	if (time.isActive())
		time.stop();
	Stop();
}

void FIR::TimeOut()
{
	--restTime;
	if (restTime == 0)
	{
		if (!quitMBox->isHidden())
		{
			quitMBox->setHidden(true);
			emit readyCode(254);
		}
	}
	if (turn)
	{
		ui->localtime->display(restTime);
		++localTime;
		if (restTime == 0)
		{
			time.stop();
			restTime = interval;
			if (!loseMBox->isHidden())
			{
				loseMBox->setHidden(true);
				Stop();
				emit readyCode(126);
				return;
			}
			emit readyCode(253);
		}
	}else
	{
		ui->peertime->display(restTime);
		++peerTime;
		if (restTime == 0)
		{
			if (!withdrawMBox->isHidden())
			{
				withdrawMBox->setHidden(true);
				emit readyCode(251);
			}
			restTime = interval;
			time.stop();
		}
	}
}

void FIR::Decode(QString &str)
{
	str.append(" -1 -1 -1");
	QStringList strList = str.split(" ");
	int mode = strList[0].toInt();
	int x, y;
	bool flag;
	switch (mode)
	{
	case 1://set a point
		x = strList[1].toInt();
		y = strList[2].toInt();
		if (isLegalPoint(x, y) && map[x][y] == 0)
		{
			turnChange();
			ui->textBrowser->append(tr("Your peer puts a chessman."));
			put.push_back(QPoint(x, y));
			map[x][y] = 3 - color;
			if (isWin(x, y))
				Lose();
			update();
		}
		break;
	case 2://asked to start
		ui->textBrowser->append(tr("Your peer asks for start."));
		if (!listen)
		{
			x = strList[1].toInt();
			if (x == 0)
				turn = false;
			else
				turn = true;
			color = x + 1;
			for (int i = 0; i <= tScale; ++i)
				temp[i] = strList[i + 2].toInt();
			if (turn)
				color = temp[tScale];
			else
				color = 3 - temp[tScale];
		}
		if (startHaveAsk)
			Start();
		startAsked = true;
		break;
	case 126://the peer agree with the result of lose
		ui->textBrowser->append(tr("Your peer agrees with the result of lose."));
		Stop();
		break;
	case 127://get agreement to withdraw
		ui->textBrowser->append(tr("You get the agreement to withdraw."));
		Withdraw(color);
		++withdrawNum;
		break;
	case 128://get agreement with the connection
		ui->textBrowser->append("Linked " + socket->peerAddress().toString()
				   + ":" + QString::number(socket->peerPort()));
		connected = true;
		ui->pushButton_4->setDisabled(true);
		break;
	case 255://break the connect
		if (start && !breakHaveAsk)
		{
			quitMBox->setHidden(false);
			return;
		}
		if (breakHaveAsk)
		{
			breakHaveAsk = false;
			connected = false;
			BreakConnect();
		}
		break;
	case 254://reject to break
		breakHaveAsk = false;
		break;
	case 253://the peer runs out of time
		ui->textBrowser->append(tr("Your peer runs out of time."));
		turnChange();
		break;
	case 252://the peer asks for withdraw
		flag = false;
		foreach (QPoint p, put)
			if (map[p.x()][p.y()] != color)
			{
				flag = true;
				break;
			}
		if (startHaveAsk && flag)
			withdrawMBox->setHidden(false);
		break;
	case 251:
		ui->textBrowser->append(tr("Your peer rejects your application to withdraw."));
		if (end)
		{
			Stop();
			emit readyCode(126);
		}else
		start = true;
		break;
	}
}

void FIR::Code(int mode, int x, int y)
{
	QByteArray arr;
	arr.clear();
	arr.append("#");
	switch (mode)
	{
	case 1://put point
		ui->textBrowser->append(tr("You set a chessman"));
		withdrawNum = 0;
		put.push_back(QPoint(x, y));
		arr.append(QString::number(mode) + " " +
				   QString::number(x) + " " +
				   QString::number(y));
		turnChange();
		break;
	case 2://start
		if (!startHaveAsk)
		{
			startHaveAsk = true;
			arr.append(QString::number(mode));
			if (listen)
			{
				x = rand();
				x %= 2;
				if (x == 0)
					turn = true;
				else
					turn = false;
				arr.append(" " + QString::number(x));
				int *array = bak[ui->listView->currentIndex().row()];
				for (int i = 0; i <= tScale; ++i)
				{
					temp[i] = array[i];
					arr.append(" " + QString::number(temp[i]));
				}
				if (turn)
					color = temp[tScale];
				else
					color = 3 - temp[tScale];
			}
			ui->textBrowser->append(tr("You ask to start."));
			if (startAsked)
				Start();
		} else
			return;
		break;
	case 128://agree
		arr.append("128");
		ui->textBrowser->append("Linked " + socket->peerAddress().toString() +
				   ":" + QString::number(socket->peerPort()));
		break;
	case 255://require break
		arr.append("255");
		breakHaveAsk = true;
		ui->textBrowser->append(tr("You ask for break."));
		break;
	case 253://timeout
		ui->textBrowser->append(tr("You run out of your time."));
		turnChange();
	default:
		arr.append(QString::number(mode));
	}
	arr.append("#");
	socket->write(arr);
}

void FIR::conFinished()
{
	switch (connectMBox->result())
	{
	case QMessageBox::Ok:
		socket = server->nextPendingConnection();
		connect(socket, SIGNAL(readyRead()),
				this, SLOT(Receive()));
		ui->pushButton->setDisabled(true);
		connected = true;
		ui->pushButton_4->setDisabled(true);
		emit readyCode(128);
		return;
	default:
		QTcpSocket *s = server->nextPendingConnection();
		s->write("#255#");
		s->disconnectFromHost();
		delete s;
	}
}

void FIR::loseFinished()
{

	switch (loseMBox->result())
	{
	case QMessageBox::Ignore:
		if (on_Withdraw_clicked())
			break;
	case QMessageBox::Ok:
		Stop();
		emit readyCode(126);
		break;
	}
}

void FIR::withFinished()
{
	switch (withdrawMBox->result())
	{
	case QMessageBox::Ok:
		Withdraw(3 - color);
		emit readyCode(127);
		end = false;
		break;
	case QMessageBox::Ignore:
		if (end)
			Stop();
		emit readyCode(251);
		break;
	}
}

void FIR::quitFinished()
{

	switch (quitMBox->result())
	{
	case QMessageBox::Ok:
		start = false;
		time.stop();
		on_pushButton_9_clicked();
		break;
	case QMessageBox::Ignore:
		emit readyCode(254);
		break;
	}
}

//function
bool FIR::getPoint(int &x, int &y)
{
	int i = 0;
	x -= space;
	y -= space;
	for (i = 0; i <= scale; ++i)
		if (x >= cols[i] && x < cols[i + 1])
			break;
	int xx = i;
	if (xx > scale) return false;
	for (i = 0; i <= scale; ++i)
		if (y >= rows[i] && y < rows[i + 1])
			break;
	int yy = i;
	if (yy > scale)
		return false;
	qreal d2 = len * len * 0.48 * 0.48;
	int d[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
	for (i = 0; i < 4; ++i)
	{
		qreal dx = x - cols[xx + d[i][0]];
		qreal dy = y - rows[yy + d[i][1]];
		if (dx * dx + dy * dy < d2)
		{
			x = xx + d[i][0] - 1;
			y = yy + d[i][1] - 1;
			return true;
		}
	}
	return false;
}

bool FIR::isWin(int x, int y)
{
	int d[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
	for (int i = 0; i < 4; ++i)
	{
		int s = -1;
		int u = x, v = y;
		while (isLegalPoint(u, v) &&
			   map[u][v] == map[x][y])
		{
			u += d[i][0];
			v += d[i][1];
			++s;
		}
		u = x;
		v = y;
		while (u >= 0 && u < scale &&
			   v >= 0 && v < scale &&
			   map[u][v] == map[x][y])
		{
			u -= d[i][0];
			v -= d[i][1];
			++s;
		}
		if (s >= 5)
		{
			ui->textBrowser->append(tr("The time you use:") + zero.addSecs(localTime).toString());
			ui->textBrowser->append(tr("the time your peer uses:") + zero.addSecs(peerTime).toString());
			end = true;
			return true;
		}
	}
	return false;
}

bool FIR::isLegalPoint(int x, int y)
{
	return x >= 0 && x < scale &&
		   y >= 0 && y < scale;
}

void FIR::Start()
{
	put.clear();
	end = false;
	withdrawNum = 0;
	localTime = 0;
	peerTime = 0;
	restTime = interval;
	start = true;
	breakHaveAsk = false;
	for (int i = 0; i < tScale; ++i)
		map[i % scale][i / scale] = temp[i];;
	if (color == 1)
		ui->Color->setText("White");
	else
		ui->Color->setText("Black");
	if (turn)
		ui->State->setText("It's your turn.");
	else
		ui->State->setText("It's your peer's turn.");
	ui->localtime->display(interval);
	ui->peertime->display(interval);
	time.start();
	update();
}

void FIR::Stop()
{
	time.stop();
	turn = false;
	start = false;
	startHaveAsk = false;
	startAsked = false;
	ui->State->setText("Free");
	ui->Color->setText("     ");
	breakHaveAsk = false;
	update();
}

void FIR::turnChange()
{
	time.stop();
	restTime = interval;
	if (turn)
	{
		ui->localtime->display(restTime);
		ui->State->setText(tr("It's your peer's turn."));
	}else
	{
		ui->peertime->display(restTime);
		ui->State->setText(tr("It's your turn"));
	}
	turn = !turn;
	time.start();
}

void FIR::Win()
{
	ui->textBrowser->append("You win!");
	start = false;
	soundWin->seek(0);
	audioWin->start(soundWin);
}

void FIR::Lose()
{
	ui->textBrowser->append(tr("You lost!"));
	start = false;
	soundLose->seek(0);
	audioLose->start(soundLose);
	loseMBox->setHidden(false);
}

void FIR::Withdraw(int t_color)
{
	start = true;
	end = false;
	int l = put.length() - 1;
	while (map[put[l].x()][put[l].y()] != t_color && l >= 0)
	{
		map[put[l].x()][put[l].y()] = 0;
		put.pop_back();
		--l;
	}
	if (l >= 0)
	{
		map[put[l].x()][put[l].y()] = 0;
		put.pop_back();
	}
	update();
}

void FIR::Load()
{
	QFile in("bak.bak");
	in.open(QIODevice::ReadOnly | QIODevice::Text);
	QTextStream txtInput(&in);
	QString str;
	strList.clear();
	strList.append("New");
	int *arr = new int[tScale + 1];
	for (int i = 0; i < tScale; ++i)
		arr[i] = 0;
	arr[tScale] = 2;
	bak.push_back(arr);
	str = txtInput.readAll();
	QStringList bakStr = str.split("@");
	for (int i = 0; i < bakStr.length(); ++i)
	{
		str = bakStr[i];
		if (str == "")
			continue;
		QStringList l = str.split(" ");
		strList.append(l[0] + " " + l[1]);
		arr = new int[tScale + 1];
		for (int i = 0; i <= tScale; ++i)
			arr[i] = l[i + 2].toInt();
		bak.push_back(arr);
	}
	model= new QStringListModel(this);
	model->setStringList(strList);
	ui->listView->setModel(model);
	ui->listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	in.close();
}

void FIR::setMBox(QMessageBox *MBox, fun ptr, QString str, QString sub)
{
	MBox->addButton(QMessageBox::Ok);
	MBox->addButton(QMessageBox::Ignore);
	MBox->setText(str);
	if (sub != "")
		MBox->setButtonText(QMessageBox::Ignore, sub);
	MBox->setHidden(true);
	connect(MBox, &QMessageBox::finished,
			this, ptr);
}

//virtual
void FIR::paintEvent(QPaintEvent *ev)
{
	QPainter p(this);
	p.drawPixmap(0, 0, width(), height(), QPixmap(":/picture/back"));
	p.translate(space, space);
	for (int i = 1; i <= scale; ++i)
	{
		p.drawLine(cols[1], rows[i], cols[scale], rows[i]);
		p.drawLine(cols[i], rows[1], cols[i], rows[scale]);
	}
	QPen pen(Qt::black, len * 0.35);
	pen.setJoinStyle(Qt::RoundJoin);
	pen.setCapStyle(Qt::RoundCap);
	p.setPen(pen);
	int point[5][2] = {{4, 4}, {4, 12}, {12, 4}, {12, 12}, {8, 8}};
	for (int i = 0; i < 5; ++i)
		p.drawPoint(cols[point[i][0]], rows[point[i][1]]);
	pen.setWidth(len * 0.90);
	for (int i = 0; i < scale; ++i)
		for (int j = 0; j < scale; ++j)
		{
			int k = map[i][j];
			if (k != 0)
			{
				if (k == 2)
					pen.setColor(Qt::black);
				else
					pen.setColor(Qt::white);
				p.setPen(pen);
				p.drawPoint(cols[i + 1], rows[j + 1]);
			}
		}
	QWidget::paintEvent(ev);
}

void FIR::mousePressEvent(QMouseEvent *ev)
{
	int x = ev->x(), y = ev->y();
	if (start && turn && ev->button() == Qt::LeftButton)
	{
		if (getPoint(x, y) && map[x][y] == 0)
		{
			map[x][y] = color;
			emit readyCode(1, x, y);
			if (isWin(x, y))
				Win();
			update();
		}
	}
	QWidget::mousePressEvent(ev);
}

void FIR::closeEvent(QCloseEvent *ev)
{
	ui->widget->setHidden(true);
	ui->widget_2->setHidden(true);
	ui->widget_3->setHidden(true);
	QWidget::closeEvent(ev);
}
