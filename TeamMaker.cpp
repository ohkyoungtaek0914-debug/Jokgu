#include "TeamMaker.h"

#include <QtMath>

#include <algorithm>
#include <limits>
#include <random>

namespace {

constexpr double kNearAbsDefault = 0.50;
constexpr double kNearRelDefault = 0.10;
constexpr int kNearTopKDefault = 200;
constexpr int kFallbackTopN = 10;

std::mt19937& globalRng()
{
    static std::mt19937 rng(std::random_device{}());
    return rng;
}

struct EvalResult {
    QVector<QVector<PlayerInfo>> teams;
    QVector<double> teamSums;
    TeamMetrics metrics;
};

struct BestPool {
    double bestScore = std::numeric_limits<double>::max();
    double nearAbs = kNearAbsDefault;
    double nearRel = kNearRelDefault;
    int nearTopK = kNearTopKDefault;
    QVector<EvalResult> candidates;

    bool isNear(double score) const
    {
        if (bestScore == std::numeric_limits<double>::max()) {
            return true;
        }
        const double absGap = score - bestScore;
        const double relBase = qMax(qAbs(bestScore), 1e-9);
        const double relGap = absGap / relBase;
        return absGap <= nearAbs || relGap <= nearRel;
    }
};

double round1(double value)
{
    return qRound(value * 10.0) / 10.0;
}

double clamp01(double value)
{
    return qBound(0.0, value, 1.0);
}

double calculateWinRateScore(int wins, int losses)
{
    const int games = wins + losses;
    const double raw = (games > 0)
        ? static_cast<double>(wins) / static_cast<double>(games)
        : 0.5;
    const double confidence = clamp01(static_cast<double>(games) / 10.0);
    const double rate = 0.5 * (1.0 - confidence) + raw * confidence;
    const double score = 1.0 + 4.0 * rate;
    return round1(qBound(1.0, score, 5.0));
}

} // namespace

TeamMaker::TeamMaker()
{
    const QVector<QPair<QString, double>> players_base = {
        {QStringLiteral(u"고한솔"), 4},
        {QStringLiteral(u"기대현"), 3},
        {QStringLiteral(u"김도헌"), 2},
        {QStringLiteral(u"김동윤"), 4},
        {QStringLiteral(u"김세현"), 5},
        {QStringLiteral(u"문지환"), 3},
        {QStringLiteral(u"박승우"), 4},
        {QStringLiteral(u"박주환"), 5},
        {QStringLiteral(u"박준혁"), 4},
        {QStringLiteral(u"양재원"), 5},
        {QStringLiteral(u"오경택"), 4},
        {QStringLiteral(u"이규빈"), 3},
        {QStringLiteral(u"이상오"), 5},
        {QStringLiteral(u"이종균"), 5},
        {QStringLiteral(u"이재상"), 5},
        {QStringLiteral(u"이창준"), 4},
        {QStringLiteral(u"유경두"), 5},
    };

    const QMap<QString, QPair<int, int>> records = {
        {QStringLiteral(u"고한솔"), {4, 10}},
        {QStringLiteral(u"기대현"), {7, 6}},
        {QStringLiteral(u"김도헌"), {3, 1}},
        {QStringLiteral(u"김동윤"), {5, 2}},
        {QStringLiteral(u"김세현"), {9, 3}},
        {QStringLiteral(u"문지환"), {2, 2}},
        {QStringLiteral(u"박승우"), {8, 3}},
        {QStringLiteral(u"박주환"), {6, 5}},
        {QStringLiteral(u"박준혁"), {7, 6}},
        {QStringLiteral(u"양재원"), {9, 4}},
        {QStringLiteral(u"오경택"), {6, 8}},
        {QStringLiteral(u"이규빈"), {1, 0}},
        {QStringLiteral(u"이상오"), {1, 4}},
        {QStringLiteral(u"이종균"), {3, 3}},
        {QStringLiteral(u"이재상"), {2, 1}},
        {QStringLiteral(u"이창준"), {4, 8}},
        {QStringLiteral(u"유경두"), {5, 1}},
    };

    m_players.reserve(players_base.size());
    for (const auto& p : players_base) {
        const auto rec = records.value(p.first, {0, 0});
        PlayerInfo info;
        info.name = p.first;
        info.skillScore = p.second;
        info.wins = rec.first;
        info.losses = rec.second;
        info.games = info.wins + info.losses;
        info.winRateScore = calculateWinRateScore(info.wins, info.losses);
        info.finalScore = round1(info.skillScore + info.winRateScore);
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
    return selectedCount >= 11 ? 3 : 2;
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
        for (const auto& p : team) {
            scores.push_back(p.finalScore);
        }
        std::sort(scores.begin(), scores.end(), std::greater<double>());

        double top2 = 0.0;
        double bottom2 = 0.0;
        for (int i = 0; i < std::min(2, scores.size()); ++i) {
            top2 += scores[i];
            bottom2 += scores[scores.size() - 1 - i];
        }

        top2Sums.push_back(top2);
        bottom2Sums.push_back(bottom2);
    }

    m.diffTop2 = round1(*std::max_element(top2Sums.begin(), top2Sums.end())
                        - *std::min_element(top2Sums.begin(), top2Sums.end()));
    m.diffBottom2 = round1(*std::max_element(bottom2Sums.begin(), bottom2Sums.end())
                           - *std::min_element(bottom2Sums.begin(), bottom2Sums.end()));

    double avgSum = 0.0;
    for (double s : teamSums) {
        avgSum += s;
    }
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

    std::shuffle(selectedPlayers.begin(), selectedPlayers.end(), globalRng());
    std::stable_sort(selectedPlayers.begin(), selectedPlayers.end(), [](const PlayerInfo& a, const PlayerInfo& b) {
        if (!qFuzzyCompare(a.finalScore + 1.0, b.finalScore + 1.0)) {
            return a.finalScore > b.finalScore;
        }
        return a.name < b.name;
    });

    const int teamCount = decide_team_count(selectedPlayers.size());
    QVector<QVector<PlayerInfo>> teams(teamCount);
    QVector<double> teamSums(teamCount, 0.0);

    BestPool pool;
    QVector<EvalResult> allResults;
    auto searchBestPartitionRec = [&](auto&& self,
                                      int playerIndex,
                                      QVector<QVector<PlayerInfo>>& curTeams,
                                      QVector<double>& curSums) -> void {
        if (playerIndex >= selectedPlayers.size()) {
            EvalResult cur;
            cur.teams = curTeams;
            cur.teamSums = curSums;
            cur.metrics = calculateMetrics(cur.teams, cur.teamSums);

            allResults.push_back(cur);

            if (cur.metrics.score + 1e-9 < pool.bestScore) {
                pool.bestScore = cur.metrics.score;
                pool.candidates.clear();
                pool.candidates.push_back(cur);
            } else if (pool.isNear(cur.metrics.score)) {
                pool.candidates.push_back(cur);
            }
            return;
        }

        const PlayerInfo& p = selectedPlayers[playerIndex];
        for (int t = 0; t < curTeams.size(); ++t) {
            curTeams[t].push_back(p);
            curSums[t] += p.finalScore;

            self(self, playerIndex + 1, curTeams, curSums);

            curSums[t] -= p.finalScore;
            curTeams[t].pop_back();
        }
    };
    searchBestPartitionRec(searchBestPartitionRec, 0, teams, teamSums);

    if (pool.candidates.isEmpty()) {
        return result;
    }

    if (pool.candidates.size() > pool.nearTopK) {
        std::nth_element(pool.candidates.begin(),
                         pool.candidates.begin() + pool.nearTopK,
                         pool.candidates.end(),
                         [](const EvalResult& a, const EvalResult& b) {
                             return a.metrics.score < b.metrics.score;
                         });
        pool.candidates.resize(pool.nearTopK);
    }

    QVector<EvalResult> candidatePool = pool.candidates;
    if (pool.candidates.size() <= 2 && !allResults.isEmpty()) {
        std::sort(allResults.begin(), allResults.end(), [](const EvalResult& a, const EvalResult& b) {
            return a.metrics.score < b.metrics.score;
        });
        const int fallbackCount = std::min(kFallbackTopN, allResults.size());
        candidatePool = allResults.mid(0, fallbackCount);
    }

    std::uniform_int_distribution<int> dist(0, candidatePool.size() - 1);
    const int pickedCandidateIndex = dist(globalRng());
    const EvalResult picked = candidatePool[pickedCandidateIndex];

    result.metrics = picked.metrics;
    result.metrics.bestScore = round1(pool.bestScore);
    result.metrics.candidateCount = candidatePool.size();
    result.metrics.selectedCandidateIndex = pickedCandidateIndex;
    result.metrics.nearAbs = pool.nearAbs;
    result.metrics.nearRel = pool.nearRel;
    result.metrics.nearTopK = pool.nearTopK;

    result.teamSums.reserve(picked.teamSums.size());
    for (double sum : picked.teamSums) {
        result.teamSums.push_back(round1(sum));
    }

    for (int teamIdx = 0; teamIdx < picked.teams.size(); ++teamIdx) {
        const double sum = round1(picked.teamSums[teamIdx]);
        for (const auto& p : picked.teams[teamIdx]) {
            result.rows.push_back({teamIdx + 1, p.name, round1(p.finalScore), sum});
        }
    }

    std::sort(result.rows.begin(), result.rows.end(), [](const TeamPlayer& a, const TeamPlayer& b) {
        if (a.teamIndex != b.teamIndex) {
            return a.teamIndex < b.teamIndex;
        }
        return a.finalScore > b.finalScore;
    });

    return result;
}
