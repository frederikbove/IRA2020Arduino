#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTextEdit>
#include <QtWidgets>
#include <QDebug>
#include <QLabel>
#include <QMessageBox>
#include <QTableWidget>
#include <QMainWindow>

#include "natsclient.h"
#include "ira2020.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void natsAnnounce();
    void natsDisconnected();
    void natsConnected();

private slots:
    void connectClicked();
    void pingClicked();

private:
    void buildGUI();
    void createStatusBar();

    QLineEdit*      server_line_edit;
    QLineEdit*      port_line_edit;
    QPushButton*    connect_button;
    QPushButton*    ping_button;
    QTextEdit*      text_edit;

    QTableWidget*     device_table;

    Nats::Client                client;
    Nats::Options               options;
    Nats::Subscription          *announce_subscription;
    QVector<Area3001::Ira2020>  device_vector;
};
#endif // MAINWINDOW_H
