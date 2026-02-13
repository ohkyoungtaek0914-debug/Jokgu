#include "TeamMaker.h"

#include <QDateTime>
#include <QRandomGenerator>
#include <QtMath>

#include <algorithm>
#include <limits>

namespace {

double winRateAdjustment(int games, int wins)
{
    if (games <= 0) {
        return 0.0;
    }

    const double winRate = static_cast<double>(wins) / static_cast<double>(games);
    if (winRate >= 0.8) return 1.0;
    if (winRate >= 0.65) return 0.6;
    if (winRate >= 0.55) return 0.3;
    if (winRate >= 0.45) return 0.0;
    if (winRate >= 0.35) return -0.3;
    if (winRate >= 0.2) return -0.6;
    return -1.0;
}

double round1(double value)
{
    return qRound(value * 10.0) / 10.0;
}

} // namespace

TeamMaker::TeamMaker()
{
    QRandomGenerator::global()->seed(static_cast<quint32>(QDateTime::currentMSecsSinceEpoch() & 0xffffffff));

    const QVector<QPair<QString, double>> players_base = {
        {"고한솔", 5},
        {"기대현", 3},
        {"김도헌", 2},
        {"김동윤", 4},
        {"김세현", 5},
        {"문지환", 3},
        {"박승우", 3},
        {"박주환", 5},
        {"박준혁", 4},
        {"양재원", 4},
        {"오경택", 4},
        {"이규빈", 3},
        {"이상오", 5},
        {"이종균", 5},
        {"이재상", 5},
        {"이창준", 4},
        {"조재경", 2}
    };

    const QMap<QString, QPair<int, int>> records = {
        {"고한솔", {12, 3}},
        {"기대현", {7, 7}},
        {"김도헌", {0, 1}},
        {"김동윤", {6, 5}},
        {"김세현", {10, 4}},
        {"문지환", {0, 2}},
        {"박승우", {2, 1}},
        {"박주환", {6, 9}},
        {"박준혁", {1, 1}},
        {"양재원", {7, 5}},
        {"오경택", {10, 5}},
        {"이규빈", {1, 0}},
        {"이상오", {2, 4}},
        {"이종균", {3, 4}},
        {"이재상", {2, 1}},
        {"이창준", {2, 9}},
        {"조재경", {0, 1}}
    };

    m_players.reserve(players_base.size());
    for (const auto& p : players_base) {
        const auto rec = records.value(p.first, {0, 0});
        PlayerInfo info;
        info.name = p.first;
        info.baseScore = p.second;
        info.games = rec.first;
        info.wins = rec.second;
        info.adjustedScore = round1(info.baseScore + winRateAdjustment(info.games, info.wins));
        m_players.push_back(info);
        m_playerByName.insert(info.name, info);
    }
}

QVector<PlayerInfo> TeamMaker::allPlayers() const
{
    return m_players;
}

int TeamMaker::decide_team_count(int selectedCount) const
{
    return selectedCount >= 9 ? 3 : 2;
}

TeamMetrics TeamMaker::calculateMetrics(const QVector<QVector<PlayerInfo>>& teams,
                                        const QVector<double>& teamSums) const
{
    TeamMetrics m{};

    if (teamSums.isEmpty()) {
        return m;
    }

    double maxSum = *std::max_element(teamSums.begin(), teamSums.end());
    double minSum = *std::min_element(teamSums.begin(), teamSums.end());
    m.diffSum = round1(maxSum - minSum);

    QVector<double> avgs;
    avgs.reserve(teams.size());
    for (int i = 0; i < teams.size(); ++i) {
        double avg = teams[i].isEmpty() ? 0.0 : teamSums[i] / teams[i].size();
        avgs.push_back(avg);
    }
    const double maxAvg = *std::max_element(avgs.begin(), avgs.end());
    const double minAvg = *std::min_element(avgs.begin(), avgs.end());
    m.diffAvg = round1(maxAvg - minAvg);

    QVector<double> top2Sums;
    QVector<double> bottom2Sums;
    top2Sums.reserve(teams.size());
    bottom2Sums.reserve(teams.size());

    for (const auto& team : teams) {
        QVector<double> scores;
        for (const auto& p : team) scores.push_back(p.adjustedScore);
        std::sort(scores.begin(), scores.end(), std::greater<double>());

        double top2 = 0.0;
        double bottom2 = 0.0;
        for (int i = 0; i < std::min(2, scores.size()); ++i) top2 += scores[i];
        for (int i = 0; i < std::min(2, scores.size()); ++i) bottom2 += scores[scores.size() - 1 - i];

        top2Sums.push_back(top2);
        bottom2Sums.push_back(bottom2);
    }

    m.diffTop2 = round1(*std::max_element(top2Sums.begin(), top2Sums.end())
                        - *std::min_element(top2Sums.begin(), top2Sums.end()));
    m.diffBottom2 = round1(*std::max_element(bottom2Sums.begin(), bottom2Sums.end())
                           - *std::min_element(bottom2Sums.begin(), bottom2Sums.end()));

    double avgSum = 0.0;
    for (double s : teamSums) avgSum += s;
    avgSum /= teamSums.size();

    double variance = 0.0;
    for (double s : teamSums) {
        const double d = s - avgSum;
        variance += d * d;
    }
    variance /= teamSums.size();
    m.varSum = round1(variance);

    m.score = round1(m.diffSum + m.diffAvg + m.diffTop2 + m.diffBottom2 + m.varSum);
    return m;
}

TeamResult TeamMaker::makeTeams(const QVector<QString>& selectedNames) const
{
    TeamResult result;

    QVector<PlayerInfo> selectedPlayers;
    selectedPlayers.reserve(selectedNames.size());
    for (const QString& name : selectedNames) {
        if (m_playerByName.contains(name)) {
            selectedPlayers.push_back(m_playerByName.value(name));
        }
    }

    if (selectedPlayers.size() < 2) {
        return result;
    }

    std::sort(selectedPlayers.begin(), selectedPlayers.end(), [](const PlayerInfo& a, const PlayerInfo& b) {
        if (!qFuzzyCompare(a.adjustedScore + 1.0, b.adjustedScore + 1.0)) {
            return a.adjustedScore > b.adjustedScore;
        }
        return a.name < b.name;
    });

    const int teamCount = decide_team_count(selectedPlayers.size());
    QVector<QVector<PlayerInfo>> teams(teamCount);
    QVector<double> teamSums(teamCount, 0.0);

    for (const auto& player : selectedPlayers) {
        double bestImbalance = std::numeric_limits<double>::max();
        QVector<int> candidateTeams;

        for (int t = 0; t < teamCount; ++t) {
            QVector<double> trial = teamSums;
            trial[t] += player.adjustedScore;
            const double mx = *std::max_element(trial.begin(), trial.end());
            const double mn = *std::min_element(trial.begin(), trial.end());
            const double imbalance = mx - mn;

            if (imbalance + 1e-9 < bestImbalance) {
                bestImbalance = imbalance;
                candidateTeams = {t};
            } else if (qAbs(imbalance - bestImbalance) < 1e-9) {
                candidateTeams.push_back(t);
            }
        }

        int pickedIndex = candidateTeams[0];
        if (candidateTeams.size() > 1) {
            const int r = QRandomGenerator::global()->bounded(candidateTeams.size());
            pickedIndex = candidateTeams[r];
        }

        teams[pickedIndex].push_back(player);
        teamSums[pickedIndex] += player.adjustedScore;
    }

    result.metrics = calculateMetrics(teams, teamSums);

    for (int teamIdx = 0; teamIdx < teams.size(); ++teamIdx) {
        const double sum = round1(teamSums[teamIdx]);
        for (const auto& p : teams[teamIdx]) {
            result.rows.push_back({teamIdx + 1, p.name, round1(p.adjustedScore), sum});
        }
    }

    std::sort(result.rows.begin(), result.rows.end(), [](const TeamPlayer& a, const TeamPlayer& b) {
        if (a.teamIndex != b.teamIndex) return a.teamIndex < b.teamIndex;
        return a.adjustedScore > b.adjustedScore;
    });

    return result;
}
