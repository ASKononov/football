#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QDebug>
#include <QString>
#include <QtSql/QSqlRecord>
#include <string>
#include <qpixmap.h>
#include <QHttpPart>
#include <QDate>
#include <QLabel>
#include <QSqlError>
#include <QTableWidget>

QSqlDatabase DB;
bool F = true;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    networkManagerF = new QNetworkAccessManager();
    networkManagerT = new QNetworkAccessManager();

    QNetworkRequest RequestFixtures;
    RequestFixtures.setRawHeader("X-Auth-Token", "6de063133c3b44bdb9c2b21176b3d59e");
    QNetworkRequest RequestTeams;
    RequestTeams.setRawHeader("X-Auth-Token", "6de063133c3b44bdb9c2b21176b3d59e");
    RequestFixtures.setUrl(QUrl("http://api.football-data.org/v1/competitions/445/fixtures"));
    RequestTeams.setUrl(QUrl("http://api.football-data.org/v1/competitions/445/leagueTable"));
    networkManagerF->get(RequestFixtures);

    connect(networkManagerF, &QNetworkAccessManager::finished, this, &MainWindow::onResultFixtures);

    networkManagerT->get(RequestTeams);

    connect(networkManagerT, &QNetworkAccessManager::finished, this, &MainWindow::onResultTeams);

    DB = QSqlDatabase::addDatabase("QPSQL");
    DB.setDatabaseName("postgres");
    DB.setHostName("127.0.0.1");
    DB.setPort(5432);
    DB.setPassword("1234");
    DB.setUserName("user");

    //DB = QSqlDatabase::addDatabase("QSQLITE");
    //DB.setDatabaseName("myDB.db");

    DB.open();

    QSqlQuery query;
    query.exec("DROP TABLE Fixtures");
     query.exec("DROP TRIGGER make_prediction_insert on Fixtures");
     query.exec("DROP TRIGGER make_prediction_update on Fixtures");
     query.exec("DROP TABLE Teams");
     query.exec("DROP VIEW FixtureLogoHome");
     query.exec("DROP VIEW FixtureLogoAway");
 }

 MainWindow::~MainWindow()
 {
     QSqlQuery query;
     query.exec("DROP TRIGGER make_prediction_insert on Fixtures");
     query.exec("DROP TRIGGER make_prediction_update on Fixtures");
     delete ui;
 }

 void MainWindow::onResultFixtures(QNetworkReply *reply)
 {
    QSqlQuery query;

    QString queryStr;

    if(!reply->error())
    {
        QJsonDocument document = QJsonDocument::fromJson(reply->readAll());

        QJsonObject root = document.object();

        queryStr = "CREATE TABLE Fixtures(id integer PRIMARY KEY, HomeTeamName varchar(100), goalsHomeTeam int, goalsAwayTeam int, AwayTeamName varchar(100), Date date, status varchar(100))";
        query.exec(queryStr);

        queryStr = "CREATE TRIGGER make_prediction_insert BEFORE INSERT ON Fixtures FOR EACH ROW EXECUTE PROCEDURE make_prediction_insert()";
        if(!query.exec(queryStr))
            ui->label_2->setText(query.lastError().text());

        queryStr = "CREATE TRIGGER make_prediction_update AFTER UPDATE ON Fixtures FOR EACH ROW EXECUTE PROCEDURE make_prediction_update()";
        if(!query.exec(queryStr))
            ui->label_2->setText(query.lastError().text());

//insert into Sotr values(1,1,'Vasya', 3000), (2, 1, 'Petya', 1000), (3,2,'Varya',2500),(4,1,'Sanya',100),(5,3,'Rita',6200), (6,2,'Nastya',750),(7,3,'Nat',801), (8,3,'Lena',829), (9,2,'Anya',796),(10,1,'Anya',504)

        int id;
        QJsonValue matches = root.value("fixtures");

        if(matches.isArray())
        {
            QJsonArray ja = matches.toArray();

            for(int i = 0; i < ja.count(); i++)
            {
                QJsonObject subtree = ja.at(i).toObject();
                QString Status = subtree.value("status").toString();

                id = subtree.value("_links").toObject().value("self").toObject().value("href").toString().right(6).toInt();

                QString HomeTeamName = subtree.value("homeTeamName").toString();
                QString AwayTeamName = subtree.value("awayTeamName").toString();

                QJsonObject Scores = subtree.value("result").toObject();

                QString date = subtree.value("date").toString().left(10);
                int goalsHomeTeam;
                int goalsAwayTeam;

                goalsHomeTeam = Scores.value("goalsHomeTeam").toInt();
                goalsAwayTeam = Scores.value("goalsAwayTeam").toInt();

               //query.exec("SELECT * FROM Fixtures WHERE id = " + QString::number(id));
                //if(!query.next())
                    queryStr = "INSERT INTO Fixtures VALUES(" + QString::number(id) + ",'" + HomeTeamName + "'," + QString::number(goalsHomeTeam) + "," + QString::number(goalsAwayTeam) +
                                    ",'" + AwayTeamName + "','" + date + "','" + Status + "')";
                //else
                //{
                //    queryStr = "UPDATE Fixtures SET goalsHomeTeam = " + QString::number(goalsHomeTeam) + ",  goalsAwayTeam = " + QString::number(goalsAwayTeam) + ", date = '" + date + "', status = '" + Status +  "' WHERE id = " + QString::number(id) + " AND (date <> '" + date + "' OR status <> '" + Status + "' OR goalsHomeTeam <> " + QString::number(goalsHomeTeam) + " OR goalsAwayTeam <> " + QString::number(goalsAwayTeam) + ")";
                //}
                if(!query.exec(queryStr))
                {
                    ui->label_2->setText(query.lastError().text());
                    ui->textEdit->append(queryStr);
                }
            }
         }
    }

    if(!query.exec("CREATE VIEW FixtureLogoHome AS SELECT * FROM Fixtures JOIN Teams ON HomeTeamName = name"))
        ui->label_2->setText(query.lastError().text());
    if(!query.exec("CREATE VIEW FixtureLogoAway AS SELECT * FROM Fixtures JOIN Teams ON AwayTeamName = name"))
        ui->label_2->setText(query.lastError().text());

    if(!query.exec("SELECT FixtureLogoHome.id, FixtureLogoHome.logo, FixtureLogoHome.HomeTeamName, FixtureLogoHome.goalsHomeTeam, FixtureLogoAway.goalsAwayTeam, FixtureLogoHome.AwayTeamName, FixtureLogoAway.logo, FixtureLogoHome.date, FixtureLogoHome.status FROM FixtureLogoHome JOIN FixtureLogoAway ON (FixtureLogoHome.HomeTeamName = FixtureLogoAway.HomeTeamName AND FixtureLogoHome.AwayTeamName = FixtureLogoAway.AwayTeamName) WHERE FixtureLogoHome.status = 'FINISHED' OR FixtureLogoHome.status = 'IN_PLAY' ORDER BY FixtureLogoHome.date DESC, FixtureLogoHome.id"))
        ui->label_2->setText(query.lastError().text());


 //ORDER BY Fixtures.id DESC

    MainWindow::PrintTable(query, 0);

    query.exec("DROP VIEW FixtureLogoHome");
    query.exec("DROP VIEW FixtureLogoAway");

    reply->deleteLater();
}

void MainWindow::onResultTeams(QNetworkReply *reply)
{
    if(!reply->error())
    {
        QSqlQuery query;

        QString queryStr;

        queryStr = "CREATE TABLE Teams(name varchar(100), logo varchar(100), position int, points int)";
        query.exec(queryStr);

        QJsonDocument document = QJsonDocument::fromJson(reply->readAll());

        QJsonObject root = document.object();

        QJsonValue matches = root.value("standing");

        QString LogoPath;

        if(matches.isArray())
        {
            QJsonArray ja = matches.toArray();
            for(int i = 0; i < ja.count(); i++)
            {
                QJsonObject subtree = ja.at(i).toObject();
                QString Name = subtree.value("teamName").toString();
                LogoPath = subtree.value("crestURI").toString();
                int index = LogoPath.lastIndexOf('/');
                LogoPath = "logos/" + LogoPath.right(LogoPath.length() - index - 1);

                int Position = subtree.value("position").toInt();

                LogoPath = LogoPath.replace(LogoPath.length() - 3, 3, "jpg");

                int Points = subtree.value("points").toInt();

                query.exec("SELECT * FROM Teams WHERE name = '" + Name + "'");
                if(!query.next())
                    queryStr = "INSERT INTO Teams values('" + Name + "','" + LogoPath + "', " + QString::number(Position) + ", " + QString::number(Points) + ")";
                else
                    queryStr = "UPDATE Teams SET  Position = " + QString::number(Position) + ", Points = " + QString::number(Points) + " WHERE name = '" + Name + "' AND (Position <> " + QString::number(Position) + " OR Points <> " + QString::number(Points) + ")";
                if(!query.exec(queryStr))
                    ui->textEdit->append(query.lastError().text());
             }
         }
    }
    QSqlQuery query;
    //query.exec("SELECT * FROM Teams");
    //MainWindow::PrintTableTEST(query);
    //ui->label->setText("done");
    reply->deleteLater();
}

int  MainWindow::PrintTable(QSqlQuery p_query, int index)
{
    QTableWidget *table = new QTableWidget();

    if(index == 0)
        table = ui->tableWidget;
    if(index == 1)
        table = ui->tableWidget_2;

    QSqlRecord rec = p_query.record();

    if (rec.isEmpty())
        return 1;

    table->clear();

    p_query.last();
    int Size = p_query.at() + 1;
    p_query.first();
    table->setRowCount(Size);

    //table->setRowCount(p_query.size());

    if(index == 0)
        table->setColumnCount(9);
    if(index == 1)
    {
        table->setColumnCount(11);
        table->setColumnWidth(9, 0);
        table->setColumnWidth(10, 0);
    }

    table->setColumnWidth(0, 0);
    table->setColumnWidth(1, 81);
    table->setColumnWidth(6, 81);
    table->setColumnWidth(3, 40);
    table->setColumnWidth(4, 40);
    table->setColumnWidth(7, 100);
    table->setColumnWidth(8, 87);

    QStringList FieldsNames;

    for (int i = 0; i < 9; i++)
    {
        FieldsNames.append(qPrintable(rec.fieldName(i)));
    }

    ui->tableWidget->setHorizontalHeaderLabels(FieldsNames);

    QTableWidgetItem* Item;
    int j = 0;

    do
    {
        rec = p_query.record();

        Item = new QTableWidgetItem(rec.value(0).toString());//"FixtureLogoHome.id"
        table->setItem(j, 0, Item);

        QLabel *labHome = new QLabel();
        QPixmap myPixmapHome(rec.value(1).toString());//"FixtureLogoHome.logo"
        labHome->setPixmap( myPixmapHome );
        table->setCellWidget(j, 1, labHome);

        Item = new QTableWidgetItem(rec.value(2).toString());//"FixtureLogoHome.homeTeamName"
        table->setItem(j, 2, Item);

        Item = new QTableWidgetItem(rec.value(5).toString());//"FixtureLogoHome.awayTeamName"
        table->setItem(j, 5, Item);

        QLabel *labAway = new QLabel();
        QPixmap myPixmapAway(rec.value(6).toString());//"FixtureLogoAway.logo"
        labAway->setPixmap( myPixmapAway );
        table->setCellWidget(j, 6, labAway);

        Item = new QTableWidgetItem(rec.value(7).toString());//"FixtureLogoHome.date"
        table->setItem(j, 7, Item);

        if(index == 0)
        {
            Item = new QTableWidgetItem(rec.value(8).toString());//"FixtureLogoHome.status"
            table->setItem(j, 8, Item);

            Item = new QTableWidgetItem(rec.value(3).toString());//"FixtureLogoHome.goalsHomeTeam"
            table->setItem(j, 3, Item);

            Item = new QTableWidgetItem(rec.value(4).toString());//"FixtureLogoAway.goalsAwayTeam"
            table->setItem(j, 4, Item);

        }
        else
        {
            Item = new QTableWidgetItem(rec.value(8).toString());//"FixtureLogoHome.status"
            table->setItem(j, 8, Item);

            Item = new QTableWidgetItem(rec.value(9).toString());//"FixtureLogoHome.Position"
            table->setItem(j, 9, Item);

            Item = new QTableWidgetItem(rec.value(10).toString());//"FixtureLogoAway.Position"
            table->setItem(j, 10, Item);

            Item = new QTableWidgetItem(rec.value(3).toString());//"FixtureLogoHome.goalsHomeTeam"
            table->setItem(j, 3, Item);

            Item = new QTableWidgetItem(rec.value(4).toString());//"FixtureLogoAway.goalsAwayTeam"
            table->setItem(j, 4, Item);
        }
        j++;

    } while (p_query.next());

    return 0;
}

int  MainWindow::PrintTableTEST(QSqlQuery p_query)
{
    ui->tableWidget_3->setColumnCount(0);
    ui->tableWidget_3->setRowCount(0);
    ui->tableWidget_3->clear();
    QSqlRecord rec = p_query.record();

    int Count = rec.count();

    if (rec.isEmpty())
        return 1;

    ui->tableWidget_3->setColumnCount(Count);
    ui->tableWidget_3->setRowCount(20);// p_query.size()
    QStringList FieldsNames;

    for (int i = 0; i < Count; i++)
    {
        FieldsNames.append(qPrintable(rec.fieldName(i)));
    }

    ui->tableWidget_3->setHorizontalHeaderLabels(FieldsNames);

    QTableWidgetItem* Item;
    int j = 0;
    while (p_query.next())
    {
        rec = p_query.record();

        for (int i = 0; i < Count; i++)
        {
             Item = new QTableWidgetItem(rec.value(rec.fieldName(i)).toString());
             ui->tableWidget_3->setItem(j, i, Item);
        }
        j++;
    }

    return 0;
}

void MainWindow::on_tabWidget_tabBarClicked(int index)
{
    QSqlQuery query;
    if((index == 1) && F)
    {
        if(!query.exec("CREATE VIEW FixtureLogoHome AS SELECT * FROM Fixtures JOIN Teams ON HomeTeamName = name"))
            ui->label_2->setText(query.lastError().text());
        if(!query.exec("CREATE VIEW FixtureLogoAway AS SELECT * FROM Fixtures JOIN Teams ON AwayTeamName = name"))
            ui->label_2->setText(query.lastError().text());

        if(!query.exec("SELECT FixtureLogoHome.id, FixtureLogoHome.logo, FixtureLogoHome.HomeTeamName, FixtureLogoHome.goalsHomeTeam, FixtureLogoAway.goalsAwayTeam, FixtureLogoHome.AwayTeamName, FixtureLogoAway.logo, FixtureLogoHome.date, FixtureLogoHome.status, FixtureLogoHome.Position, FixtureLogoAway.Position FROM FixtureLogoHome JOIN FixtureLogoAway ON FixtureLogoHome.HomeTeamName = FixtureLogoAway.HomeTeamName AND FixtureLogoHome.AwayTeamName = FixtureLogoAway.AwayTeamName WHERE FixtureLogoHome.status = 'TIMED' OR FixtureLogoHome.status = 'SCHEDULED' ORDER BY FixtureLogoHome.date, FixtureLogoHome.id"))
            ui->label_2->setText(query.lastError().text());
        MainWindow::PrintTable(query, 1);

        F = false;

        query.exec("DROP VIEW FixtureLogoHome");
        query.exec("DROP VIEW FixtureLogoAway");
    }

    /*if(index == 0)
    {
        if(!query.exec("CREATE VIEW FixtureLogoHome AS SELECT * FROM Fixtures JOIN Teams ON HomeTeamName = name"))
            ui->label_2->setText(query.lastError().text());
        if(!query.exec("CREATE VIEW FixtureLogoAway AS SELECT * FROM Fixtures JOIN Teams ON AwayTeamName = name"))
            ui->label_2->setText(query.lastError().text());

        if(!query.exec("SELECT FixtureLogoHome.id, FixtureLogoHome.logo, FixtureLogoHome.HomeTeamName, FixtureLogoHome.goalsHomeTeam, FixtureLogoAway.goalsAwayTeam, FixtureLogoHome.AwayTeamName, FixtureLogoAway.logo, FixtureLogoHome.date, FixtureLogoHome.status FROM FixtureLogoHome JOIN FixtureLogoAway ON FixtureLogoHome.HomeTeamName = FixtureLogoAway.HomeTeamName AND FixtureLogoHome.AwayTeamName = FixtureLogoAway.AwayTeamName WHERE FixtureLogoHome.status = 'FINISHED' OR FixtureLogoHome.status = 'IN_PLAY' ORDER BY FixtureLogoHome.date DESC"))
            ui->label_2->setText(query.lastError().text());
        MainWindow::PrintTable(query, 0);

        query.exec("DROP VIEW FixtureLogoHome");
        query.exec("DROP VIEW FixtureLogoAway");
    }*/


    if(index == 2)
    {
        query.exec("SELECT position, logo, name, points FROM Teams ORDER BY position");
        MainWindow::PrintTableLeague(query);
    }


}


void MainWindow::on_pushButton_clicked()
{
    QSqlQuery query;
    if(!query.exec("CREATE VIEW FixtureLogoHome AS SELECT * FROM Fixtures JOIN Teams ON HomeTeamName = name"))
        ui->label_2->setText(query.lastError().text());
    if(!query.exec("CREATE VIEW FixtureLogoAway AS SELECT * FROM Fixtures JOIN Teams ON AwayTeamName = name"))
        ui->label_2->setText(query.lastError().text());

    if(!query.exec("SELECT FixtureLogoHome.id, FixtureLogoHome.logo, FixtureLogoHome.HomeTeamName, FixtureLogoHome.goalsHomeTeam, FixtureLogoAway.goalsAwayTeam, FixtureLogoHome.AwayTeamName, FixtureLogoAway.logo, FixtureLogoHome.date, FixtureLogoHome.status FROM FixtureLogoHome JOIN FixtureLogoAway ON FixtureLogoHome.HomeTeamName = FixtureLogoAway.HomeTeamName AND FixtureLogoHome.AwayTeamName = FixtureLogoAway.AwayTeamName WHERE ((FixtureLogoHome.status = 'FINISHED' OR FixtureLogoHome.status = 'IN_PLAY') AND (FixtureLogoHome.HomeTeamName = '" + ui->lineEdit->text() + "' OR FixtureLogoHome.AwayTeamName = '" + ui->lineEdit->text() + "')) ORDER BY FixtureLogoHome.date DESC, FixtureLogoHome.id"))
        ui->label_2->setText(query.lastError().text());
    MainWindow::PrintTable(query, 0);

    query.exec("DROP VIEW FixtureLogoHome");
    query.exec("DROP VIEW FixtureLogoAway");
}

int MainWindow::PrintTableLeague(QSqlQuery p_query)
{
    ui->tableWidget_3->setColumnCount(0);
    ui->tableWidget_3->setRowCount(0);
    ui->tableWidget_3->clear();
    QSqlRecord rec = p_query.record();

    int Count = rec.count();

    if (rec.isEmpty())
        return 1;

    ui->tableWidget_3->setColumnCount(Count);
    ui->tableWidget_3->setRowCount(20);// p_query.size()
    QStringList FieldsNames;

    for (int i = 0; i < Count; i++)
    {
        FieldsNames.append(qPrintable(rec.fieldName(i)));
    }

    ui->tableWidget_3->setHorizontalHeaderLabels(FieldsNames);

    QTableWidgetItem* Item;
    int j = 0;

    ui->tableWidget_3->setColumnWidth(0, 30);
    ui->tableWidget_3->setColumnWidth(2, 200);

    while (p_query.next())
    {
        rec = p_query.record();

        Item = new QTableWidgetItem(rec.value("position").toString());
        ui->tableWidget_3->setItem(j, 0, Item);

        QLabel *lab = new QLabel();
        QPixmap myPixmap(rec.value("logo").toString());
        lab->setPixmap( myPixmap );
        ui->tableWidget_3->setCellWidget(j, 1, lab);

        Item = new QTableWidgetItem(rec.value("name").toString());
        ui->tableWidget_3->setItem(j, 2, Item);

        Item = new QTableWidgetItem(rec.value("points").toString());
        ui->tableWidget_3->setItem(j, 3, Item);

        j++;
    }

    return 0;
}

/*DECLARE

 GoalsForHome float;
 GoalsAgHome float;
 ScoresHome float;

 GoalsForAway float;
 GoalsAgAway float;
 ScoresAway float;

 GoalsForHomeRel float;
 GoalsAgHomeRel float;
 ScoresHomeRel float;

 GoalsForAwayRel float;
 GoalsAgAwayRel float;
 ScoresAwayRel float;

 GoalsForWeight int;
 GoalsAgWeight int;
 ScoresWeight int;

 RatingHome float;
 RatingAway float;
 RatingHomeRel float;
 RatingAwayRel float;

 Fixture RECORD;

BEGIN
 GoalsForHome = 0;
 GoalsAgHome = 0;
 ScoresHome = 0;

 GoalsForAway = 0;
 GoalsAgAway = 0;
 ScoresAway = 0;

 GoalsForHomeRel = 0;
 GoalsAgHomeRel = 0;
 ScoresHomeRel = 0;

 GoalsForAwayRel = 0;
 GoalsAgAwayRel = 0;
 ScoresAwayRel = 0;

 GoalsForWeight = 7;
 GoalsAgWeight = 7;
 ScoresWeight = 9;
 IsHomeWeight = 7;

 RatingHome = 0;
 RatingAway = 0;
 RatingHomeRel = 0;
 RatingAwayRel = 0;

IF (NEW.status = 'TIMED') THEN
----------------------------homeTeam-----------------------------------
 FOR Fixture IN (SELECT * FROM Fixtures WHERE (status = 'FINISHED') AND (homeTeamName = NEW.homeTeamName OR awayTeamName = NEW.homeTeamName) ORDER BY date DESC LIMIT 5) LOOP
  IF (Fixture.homeTeamName = NEW.homeTeamName) THEN
   GoalsForHome = GoalsForHome + Fixture.goalsHomeTeam;
   GoalsAgHome = GoalsAgHome + Fixture.goalsAwayTeam;
   IF (Fixture.goalsHomeTeam > Fixture.goalsAwayTeam) THEN
    ScoresHome = ScoresHome + 3;
   END IF;
   IF (Fixture.goalsHomeTeam = Fixture.goalsAwayTeam) THEN
    ScoresHome = ScoresHome + 1;
   END IF;
   IF (Fixture.goalsHomeTeam < Fixture.goalsAwayTeam) THEN
    ScoresHome = ScoresHome + 0;
   END IF;
  END IF;

 IF (Fixture.awayTeamName = NEW.homeTeamName) THEN
   GoalsForHome = GoalsForHome + Fixture.goalsAwayTeam;
   GoalsAgHome = GoalsAgHome + Fixture.goalsHomeTeam;
   IF (Fixture.goalsHomeTeam > Fixture.goalsAwayTeam) THEN
    ScoresHome = ScoresHome + 0;
   END IF;
   IF (Fixture.goalsHomeTeam = Fixture.goalsAwayTeam) THEN
    ScoresHome = ScoresHome + 1;
   END IF;
   IF (Fixture.goalsHomeTeam < Fixture.goalsAwayTeam) THEN
    ScoresHome = ScoresHome + 3;
   END IF;
  END IF;
 END LOOP;
----------------------------awayTeam-----------------------------------
 FOR Fixture IN (SELECT * FROM Fixtures WHERE (status = 'FINISHED') AND (homeTeamName = NEW.awayTeamName OR awayTeamName = NEW.awayTeamName) ORDER BY date DESC LIMIT 5) LOOP
  IF (Fixture.homeTeamName = NEW.awayTeamName) THEN
   GoalsForAway = GoalsForAway + Fixture.goalsHomeTeam;
   GoalsAgAway = GoalsAgAway + Fixture.goalsAwayTeam;
   IF (Fixture.goalsHomeTeam > Fixture.goalsAwayTeam) THEN
    ScoresAway = ScoresAway + 3;
   END IF;
   IF (Fixture.goalsHomeTeam = Fixture.goalsAwayTeam) THEN
    ScoresAway = ScoresAway + 1;
   END IF;
   IF (Fixture.goalsHomeTeam < Fixture.goalsAwayTeam) THEN
    ScoresAway = ScoresAway + 0;
   END IF;
  END IF;

 IF (Fixture.awayTeamName = NEW.awayTeamName) THEN
   GoalsForAway = GoalsForAway + Fixture.goalsAwayTeam;
   GoalsAgAway = GoalsAgAway + Fixture.goalsHomeTeam;
   IF (Fixture.goalsHomeTeam > Fixture.goalsAwayTeam) THEN
    ScoresAway = ScoresAway + 0;
   END IF;
   IF (Fixture.goalsHomeTeam = Fixture.goalsAwayTeam) THEN
    ScoresAway = ScoresAway + 1;
   END IF;
   IF (Fixture.goalsHomeTeam < Fixture.goalsAwayTeam) THEN
    ScoresAway = ScoresAway + 3;
   END IF;
  END IF;
 END LOOP;

 GoalsForHomeRel = GoalsForHome / (GoalsForHome + GoalsForAway);
 GoalsForAwayRel = GoalsForAway / (GoalsForHome + GoalsForAway);

 GoalsAgHomeRel = 1 - GoalsAgHome / (GoalsAgHome + GoalsAgAway);
 GoalsAgAwayRel = 1 - GoalsAgAway / (GoalsAgHome + GoalsAgAway);

 ScoresHomeRel = ScoresHome / (ScoresHome + ScoresAway);
 ScoresAwayRel = ScoresAway / (ScoresHome + ScoresAway);

 RatingHome = GoalsForHomeRel * GoalsForWeight + GoalsAgHomeRel * GoalsAgWeight + ScoresHomeRel * ScoresWeight + 1 * IsHomeWeight;
 RatingAway = GoalsForAwayRel * GoalsForWeight + GoalsAgAwayRel * GoalsAgWeight + ScoresAwayRel * ScoresWeight + 0 * IsHomeWeight;

 RatingHomeRel = RatingHome / (RatingHome + RatingAway);
 RatingAwayRel = RatingAway / (RatingHome + RatingAway);

 NEW.goalsHomeTeam = ROUND(RatingHomeRel  * 100);
 NEW.goalsAwayTeam = ROUND(RatingAwayRel  * 100);

END IF;
return NEW;
END;*/

void MainWindow::on_pushButton_2_clicked()
{
    QSqlQuery query;
    if(!query.exec("CREATE VIEW FixtureLogoHome AS SELECT * FROM Fixtures JOIN Teams ON HomeTeamName = name"))
        ui->label_2->setText(query.lastError().text());
    if(!query.exec("CREATE VIEW FixtureLogoAway AS SELECT * FROM Fixtures JOIN Teams ON AwayTeamName = name"))
        ui->label_2->setText(query.lastError().text());
    if(!query.exec("SELECT FixtureLogoHome.id, FixtureLogoHome.logo, FixtureLogoHome.HomeTeamName, FixtureLogoHome.goalsHomeTeam, FixtureLogoAway.goalsAwayTeam, FixtureLogoHome.AwayTeamName, FixtureLogoAway.logo, FixtureLogoHome.date, FixtureLogoHome.status FROM FixtureLogoHome JOIN FixtureLogoAway ON (FixtureLogoHome.HomeTeamName = FixtureLogoAway.HomeTeamName AND FixtureLogoHome.AwayTeamName = FixtureLogoAway.AwayTeamName) WHERE FixtureLogoHome.status = 'FINISHED' OR FixtureLogoHome.status = 'IN_PLAY' ORDER BY FixtureLogoHome.date DESC, FixtureLogoHome.id"))
        ui->label_2->setText(query.lastError().text());

    MainWindow::PrintTable(query, 0);

    if(!query.exec("SELECT FixtureLogoHome.id, FixtureLogoHome.logo, FixtureLogoHome.HomeTeamName, FixtureLogoHome.goalsHomeTeam, FixtureLogoAway.goalsAwayTeam, FixtureLogoHome.AwayTeamName, FixtureLogoAway.logo, FixtureLogoHome.date, FixtureLogoHome.status, FixtureLogoHome.Position, FixtureLogoAway.Position FROM FixtureLogoHome JOIN FixtureLogoAway ON FixtureLogoHome.HomeTeamName = FixtureLogoAway.HomeTeamName AND FixtureLogoHome.AwayTeamName = FixtureLogoAway.AwayTeamName WHERE FixtureLogoHome.status = 'TIMED' OR FixtureLogoHome.status = 'SCHEDULED'  ORDER BY FixtureLogoHome.date, FixtureLogoHome.id"))
        ui->label_2->setText(query.lastError().text());

    MainWindow::PrintTable(query, 1);
    query.exec("DROP VIEW FixtureLogoHome");
    query.exec("DROP VIEW FixtureLogoAway");
}

/*DECLARE

 GoalsForHome float;
 GoalsAgHome float;
 ScoresHome float;

 GoalsForAway float;
 GoalsAgAway float;
 ScoresAway float;

 GoalsForHomeRel float;
 GoalsAgHomeRel float;
 ScoresHomeRel float;

 GoalsForAwayRel float;
 GoalsAgAwayRel float;
 ScoresAwayRel float;

 GoalsForWeight int;
 GoalsAgWeight int;
 ScoresWeight int;
 IsHomeWeight int;

 RatingHome float;
 RatingAway float;
 RatingHomeRel float;
 RatingAwayRel float;

 Fixture RECORD;
 FixtureUpd RECORD;
BEGIN
 IF (TG_OP = 'UPDATE') AND ((NEW.status = 'FINISHED') OR (NEW.status = 'IN_PLAY')) THEN

 FOR FixtureUpd IN (SELECT * FROM Fixtures WHERE (status = 'TIMED' OR status = 'SCHEDULED') AND (homeTeamName = NEW.homeTeamName OR awayTeamName = NEW.homeTeamName OR awayTeamName = NEW.awayTeamName OR homeTeamName = NEW.awayTeamName) ORDER BY date) LOOP
  GoalsForHome = 0;
  GoalsAgHome = 0;
  ScoresHome = 0;

  GoalsForAway = 0;
  GoalsAgAway = 0;
  ScoresAway = 0;

  GoalsForHomeRel = 0;
  GoalsAgHomeRel = 0;
  ScoresHomeRel = 0;

  GoalsForAwayRel = 0;
  GoalsAgAwayRel = 0;
  ScoresAwayRel = 0;

  GoalsForWeight = 7;
  GoalsAgWeight = 7;
  ScoresWeight = 9;
  IsHomeWeight = 7;

  RatingHome = 0;
  RatingAway = 0;
  RatingHomeRel = 0;
  RatingAwayRel = 0;

  FOR Fixture IN (SELECT * FROM Fixtures WHERE (status = 'FINISHED') AND (homeTeamName = FixtureUpd.homeTeamName OR awayTeamName = FixtureUpd.homeTeamName) ORDER BY date DESC LIMIT 5) LOOP
   IF (Fixture.homeTeamName = FixtureUpd.homeTeamName) THEN
    GoalsForHome = GoalsForHome + Fixture.goalsHomeTeam;
    GoalsAgHome = GoalsAgHome + Fixture.goalsAwayTeam;
    IF (Fixture.goalsHomeTeam > Fixture.goalsAwayTeam) THEN
     ScoresHome = ScoresHome + 3;
    END IF;
     IF (Fixture.goalsHomeTeam = Fixture.goalsAwayTeam) THEN
     ScoresHome = ScoresHome + 1;
    END IF;
    IF (Fixture.goalsHomeTeam < Fixture.goalsAwayTeam) THEN
     ScoresHome = ScoresHome + 0;
    END IF;
   END IF;

 IF (Fixture.awayTeamName = FixtureUpd.homeTeamName) THEN
   GoalsForHome = GoalsForHome + Fixture.goalsAwayTeam;
   GoalsAgHome = GoalsAgHome + Fixture.goalsHomeTeam;
   IF (Fixture.goalsHomeTeam > Fixture.goalsAwayTeam) THEN
    ScoresHome = ScoresHome + 0;
   END IF;
   IF (Fixture.goalsHomeTeam = Fixture.goalsAwayTeam) THEN
    ScoresHome = ScoresHome + 1;
   END IF;
   IF (Fixture.goalsHomeTeam < Fixture.goalsAwayTeam) THEN
    ScoresHome = ScoresHome + 3;
   END IF;
  END IF;
 END LOOP;
----------------------------awayTeam-----------------------------------
 FOR Fixture IN (SELECT * FROM Fixtures WHERE (status = 'FINISHED') AND (homeTeamName = FixtureUpd.awayTeamName OR awayTeamName = FixtureUpd.awayTeamName) ORDER BY date DESC LIMIT 5) LOOP
  IF (Fixture.homeTeamName = FixtureUpd.awayTeamName) THEN
   GoalsForAway = GoalsForAway + Fixture.goalsHomeTeam;
   GoalsAgAway = GoalsAgAway + Fixture.goalsAwayTeam;
   IF (Fixture.goalsHomeTeam > Fixture.goalsAwayTeam) THEN
    ScoresAway = ScoresAway + 3;
   END IF;
   IF (Fixture.goalsHomeTeam = Fixture.goalsAwayTeam) THEN
    ScoresAway = ScoresAway + 1;
   END IF;
   IF (Fixture.goalsHomeTeam < Fixture.goalsAwayTeam) THEN
    ScoresAway = ScoresAway + 0;
   END IF;
  END IF;

 IF (Fixture.awayTeamName = FixtureUpd.awayTeamName) THEN
   GoalsForAway = GoalsForAway + Fixture.goalsAwayTeam;
   GoalsAgAway = GoalsAgAway + Fixture.goalsHomeTeam;
   IF (Fixture.goalsHomeTeam > Fixture.goalsAwayTeam) THEN
    ScoresAway = ScoresAway + 0;
   END IF;
   IF (Fixture.goalsHomeTeam = Fixture.goalsAwayTeam) THEN
    ScoresAway = ScoresAway + 1;
   END IF;
   IF (Fixture.goalsHomeTeam < Fixture.goalsAwayTeam) THEN
    ScoresAway = ScoresAway + 3;
   END IF;
  END IF;
 END LOOP;

 GoalsForHomeRel = GoalsForHome / (GoalsForHome + GoalsForAway);
 GoalsForAwayRel = GoalsForAway / (GoalsForHome + GoalsForAway);

 GoalsAgHomeRel = 1 - GoalsAgHome / (GoalsAgHome + GoalsAgAway);
 GoalsAgAwayRel = 1 - GoalsAgAway / (GoalsAgHome + GoalsAgAway);

 ScoresHomeRel = ScoresHome / (ScoresHome + ScoresAway);
 ScoresAwayRel = ScoresAway / (ScoresHome + ScoresAway);

 RatingHome = GoalsForHomeRel * GoalsForWeight + GoalsAgHomeRel * GoalsAgWeight + ScoresHomeRel * ScoresWeight + 1 * IsHomeWeight;
 RatingAway = GoalsForAwayRel * GoalsForWeight + GoalsAgAwayRel * GoalsAgWeight + ScoresAwayRel * ScoresWeight + 0 * IsHomeWeight;

 RatingHomeRel = RatingHome / (RatingHome + RatingAway);
 RatingAwayRel = RatingAway / (RatingHome + RatingAway);

UPDATE Fixtures SET goalsHomeTeam = ROUND(RatingHomeRel  * 100), goalsAwayTeam = ROUND(RatingAwayRel  * 100) WHERE id = fixtureUpd.id;


 END LOOP;
END IF;

return NEW;
END;*/
