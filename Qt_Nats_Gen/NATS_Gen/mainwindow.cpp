#include <functional>
#include <QtWidgets>

#include "mainwindow.h"

void MainWindow::natsAnnounce()
{
    qDebug() << "Debug:" << Q_FUNC_INFO;

    qDebug().noquote() << "Received Announce Message:" << announce_subscription->message << announce_subscription->subject << announce_subscription->inbox;

    QJsonDocument doc = QJsonDocument::fromJson(announce_subscription->message.toUtf8());
    QJsonObject object = doc.object();
    //QVariantMap map = object.toVariantMap();

    //@TODO add check if these values are present++
    QJsonValue mac_string_jv = object.value(QString("mac_string"));
    QJsonValue ip_string_jv = object.value(QString("IP"));

    text_edit->append(QString("Found:"));
    text_edit->append(mac_string_jv.toString());

    Area3001::Ira2020* device = new Area3001::Ira2020();

    device->mac_string = mac_string_jv.toString();
    device->ip = QHostAddress(ip_string_jv.toString().toUInt()); // this needs to change to transmit 4 uints or do endianness conversion

    text_edit->append(device->ip.toString());

    QList<QTableWidgetItem*> itemlist = device_table->findItems(device->mac_string, Qt::MatchExactly);
    if(itemlist.empty()) // item doesn't exist yet
    {
        qDebug() << "Adding to the list";
    }
}

void MainWindow::natsDisconnected()
{
    qDebug() << "Debug:" << Q_FUNC_INFO;

    connect_button->setDisabled(false);
    ping_button->setDisabled(true);
    statusBar()->showMessage(tr("NATS Disconnected"));
}

void MainWindow::natsConnected()
{
    qDebug() << "Debug:" << Q_FUNC_INFO;

    announce_subscription = client.subscribe("area3001.announce");
    connect(announce_subscription, &Nats::Subscription::received, this, &MainWindow::natsAnnounce);

    connect_button->setDisabled(true);
    ping_button->setDisabled(false);
    statusBar()->showMessage(tr("NATS Connected"));
}

void MainWindow::connectClicked()
{
    qDebug() << "Debug:" << Q_FUNC_INFO;

    //client.connect("192.168.20.79", 4222);  // TODO need to use server_line_edit and port_line_edit
    client.connect(server_line_edit->text(), port_line_edit->text().toUInt());

    qDebug() << "Connecting to:" << server_line_edit->text() << " Port: " << port_line_edit->text().toUInt();

    connect(&client, &Nats::Client::connected, this, &MainWindow::natsConnected);
    connect(&client, &Nats::Client::disconnected, this, &MainWindow::natsDisconnected);
}

void MainWindow::pingClicked()
{
    qDebug() << "Debug:" << Q_FUNC_INFO;

    client.publish("area3001.ping", "ping");    // message content normally isn't handled
}

void MainWindow::buildGUI()
{
    qDebug() << "Debug:" << Q_FUNC_INFO;

    QHBoxLayout *Hlayout = new QHBoxLayout;
    QVBoxLayout *Vlayout = new QVBoxLayout;

    Hlayout->addWidget(new QLabel("Host: "));
    server_line_edit = new QLineEdit;
    server_line_edit->setText("192.168.20.79");

    QRegExp re_ip("(([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.){3}([0- 9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])");
    QRegExpValidator *validator_ip = new QRegExpValidator(re_ip, this);
    server_line_edit->setValidator(validator_ip);

    Hlayout->addWidget(server_line_edit);

    Hlayout->addWidget(new QLabel(" IP: "));
    port_line_edit = new QLineEdit;
    port_line_edit->setText("4222");

    QRegExp re_port("^([0-9]{1,4}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])$");
    QRegExpValidator *validator_port = new QRegExpValidator(re_port, this);
    port_line_edit->setValidator(validator_port);

    Hlayout->addWidget(port_line_edit);

    Vlayout->addLayout(Hlayout);

    connect_button = new QPushButton(tr("Connect"));
    Vlayout->addWidget(connect_button);
    connect(connect_button, &QPushButton::clicked, this, &MainWindow::connectClicked);

    ping_button = new QPushButton(tr("Ping"));
    Vlayout->addWidget(ping_button);
    ping_button->setDisabled(true);
    connect(ping_button, &QPushButton::clicked, this, &MainWindow::pingClicked);

    Vlayout->addWidget(new QLabel("Debug Output: "));
    text_edit = new QTextEdit;
    text_edit->setReadOnly(true);
    Vlayout->addWidget(text_edit);

    device_table = new QTableWidget(this);
    device_table->setColumnCount(3);
    Vlayout->addWidget(device_table);

    QWidget *widget = new QWidget;
    widget->setLayout(Vlayout);

    setCentralWidget(widget);
    show();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    createStatusBar();

    buildGUI();

    setWindowTitle(tr("IRA2020 NATS Generator"));
    setUnifiedTitleAndToolBarOnMac(true);
}

MainWindow::~MainWindow()
{
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}
