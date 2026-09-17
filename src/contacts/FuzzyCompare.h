#pragma once

#include <QString>
#include <QList>
#include <algorithm>
#include <numeric>

class FuzzyCompare
{
public:
    FuzzyCompare() = delete;

    static int levenshteinDistance(const QString &a, const QString &b);
    static qreal jaroWinklerDistance(const QString &a, const QString &b);

    template <typename T>
    static void sortListByWeight(QList<T> &list, const QList<qreal> &weights)
    {
        const QList<T> listCopy = list;

        QList<qreal> idx(list.size());
        std::iota(idx.begin(), idx.end(), 0);

        std::ranges::sort(idx, [&weights](const qsizetype left, const qsizetype right) {
            return weights.at(left) < weights.at(right);
        });

        std::ranges::transform(idx, list.begin(),
                               [&listCopy](const qsizetype i) { return listCopy[i]; });
    }
};
