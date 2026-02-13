#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QMessageBox>
#include <QStringList>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->EDT_FILTER, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
    connect(ui->BTN_ALL, &QPushButton::clicked, this, &MainWindow::onSelectAllVisible);
    connect(ui->BTN_NONE, &QPushButton::clicked, this, &MainWindow::onUnselectAllVisible);
    connect(ui->BTN_MAKE, &QPushButton::clicked, this, &MainWindow::onMakeTeams);

    loadPlayers();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadPlayers()
{
    ui->LIST_PLAYERS->clear();

    for (const PlayerInfo& player : m_teamMaker.allPlayers()) {
        auto *item = new QListWidgetItem(QStringLiteral("%1 (%2->%3)")
                                             .arg(player.name)
                                             .arg(player.baseScore, 0, 'f', 1)
                                             .arg(player.adjustedScore, 0, 'f', 1));
        item->setData(Qt::UserRole, player.name);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        ui->LIST_PLAYERS->addItem(item);
    }
}

void MainWindow::applyFilter(const QString& text)
{
    const QString keyword = text.trimmed();

    for (int i = 0; i < ui->LIST_PLAYERS->count(); ++i) {
        QListWidgetItem *item = ui->LIST_PLAYERS->item(i);
        const QString name = item->data(Qt::UserRole).toString();
        const bool visible = keyword.isEmpty() || name.contains(keyword, Qt::CaseInsensitive);
        item->setHidden(!visible);
    }
}

void MainWindow::onFilterChanged(const QString& text)
{
    applyFilter(text);
}

void MainWindow::onSelectAllVisible()
{
    for (int i = 0; i < ui->LIST_PLAYERS->count(); ++i) {
        QListWidgetItem *item = ui->LIST_PLAYERS->item(i);
        if (!item->isHidden()) {
            item->setCheckState(Qt::Checked);
        }
    }
}

void MainWindow::onUnselectAllVisible()
{
    for (int i = 0; i < ui->LIST_PLAYERS->count(); ++i) {
        QListWidgetItem *item = ui->LIST_PLAYERS->item(i);
        if (!item->isHidden()) {
            item->setCheckState(Qt::Unchecked);
        }
    }
}

void MainWindow::onMakeTeams()
{
    QVector<QString> selectedNames;

    for (int i = 0; i < ui->LIST_PLAYERS->count(); ++i) {
        QListWidgetItem *item = ui->LIST_PLAYERS->item(i);
        if (item->checkState() == Qt::Checked) {
            selectedNames.push_back(item->data(Qt::UserRole).toString());
        }
    }

    if (selectedNames.size() < 2) {
        QMessageBox::warning(this, QStringLiteral("선택 부족"),
                             QStringLiteral("참가자를 2명 이상 선택해 주세요."));
        return;
    }

    const TeamResult result = m_teamMaker.makeTeams(selectedNames);

    ui->TABLE_RESULT->setRowCount(result.rows.size());
    for (int row = 0; row < result.rows.size(); ++row) {
        const TeamPlayer& p = result.rows[row];

        ui->TABLE_RESULT->setItem(row, 0, new QTableWidgetItem(QString::number(p.teamIndex)));
        ui->TABLE_RESULT->setItem(row, 1, new QTableWidgetItem(p.name));
        ui->TABLE_RESULT->setItem(row, 2, new QTableWidgetItem(QString::number(p.adjustedScore, 'f', 1)));
        ui->TABLE_RESULT->setItem(row, 3, new QTableWidgetItem(QString::number(p.teamSum, 'f', 1)));
    }

    ui->TABLE_RESULT->resizeColumnsToContents();

    const TeamMetrics& m = result.metrics;
    ui->TXT_LOG->setPlainText(
        QStringLiteral("diff_sum: %1\n"
                       "diff_avg: %2\n"
                       "diff_top2: %3\n"
                       "diff_bottom2: %4\n"
                       "var_sum: %5\n"
                       "score: %6\n"
                       "bestScore: %7\n"
                       "candidateCount: %8\n"
                       "nearAbs: %9\n"
                       "nearRel: %10\n"
                       "K: %11")
            .arg(m.diffSum, 0, 'f', 1)
            .arg(m.diffAvg, 0, 'f', 1)
            .arg(m.diffTop2, 0, 'f', 1)
            .arg(m.diffBottom2, 0, 'f', 1)
            .arg(m.varSum, 0, 'f', 1)
            .arg(m.score, 0, 'f', 1)
            .arg(m.bestScore, 0, 'f', 1)
            .arg(m.candidateCount)
            .arg(m.nearAbs, 0, 'f', 2)
            .arg(m.nearRel, 0, 'f', 2)
            .arg(m.nearTopK));
}
