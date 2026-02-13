#ifndef TEAMMAKER_H
#define TEAMMAKER_H

#include <QMap>
#include <QString>
#include <QVector>

struct PlayerInfo {
    QString name;
    double baseScore;
    int games;
    int wins;
    double adjustedScore;
};

struct TeamPlayer {
    int teamIndex;
    QString name;
    double adjustedScore;
    double teamSum;
};

struct TeamMetrics {
    double diffSum;
    double diffAvg;
    double diffTop2;
    double diffBottom2;
    double varSum;
    double score;
};

struct TeamResult {
    QVector<TeamPlayer> rows;
    TeamMetrics metrics;
};

class TeamMaker
{
public:
    TeamMaker();

    QVector<PlayerInfo> allPlayers() const;
    TeamResult makeTeams(const QVector<QString>& selectedNames) const;

private:
    int decide_team_count(int selectedCount) const;
    TeamMetrics calculateMetrics(const QVector<QVector<PlayerInfo>>& teams,
                                 const QVector<double>& teamSums) const;

    QVector<PlayerInfo> m_players;
    QMap<QString, PlayerInfo> m_playerByName;
};

#endif // TEAMMAKER_H
