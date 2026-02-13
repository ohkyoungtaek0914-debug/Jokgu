#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "TeamMaker.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QListWidgetItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onFilterChanged(const QString& text);
    void onSelectAllVisible();
    void onUnselectAllVisible();
    void onMakeTeams();

private:
    void loadPlayers();
    void applyFilter(const QString& text);

    Ui::MainWindow *ui;
    TeamMaker m_teamMaker;
};

#endif // MAINWINDOW_H
