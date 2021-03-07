#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QSqlQuery>
#include <QString>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:

    void onResultTeams(QNetworkReply *reply);
    void onResultFixtures(QNetworkReply *reply);
    int PrintTable(QSqlQuery p_query, int index);
    int PrintTableTEST(QSqlQuery p_query);
    int PrintTableLeague(QSqlQuery p_query);

    void on_tabWidget_tabBarClicked(int index);

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManagerF;
    QNetworkAccessManager *networkManagerT;
};

#endif // MAINWINDOW_H
