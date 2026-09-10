#include <numeric>
#include <QVarLengthArray>
#include "FuzzyCompare.h"

int FuzzyCompare::levenshteinDistance(const QString &a, const QString &b)
{
    const int aSize = a.size();
    const int bSize = b.size();

    if (aSize == 0) {
        return bSize;
    }
    if (bSize == 0) {
        return aSize;
    }

    QVarLengthArray<int, 128> v0(bSize + 1);
    QVarLengthArray<int, 128> v1(bSize + 1);

    std::iota(v0.begin(), v0.end(), 0);

    for (int i = 0; i < aSize; ++i) {
        v1[0] = i + 1;

        const QChar aCharLower = a[i].toLower();

        for (int j = 0; j < bSize; ++j) {
            const int deletionCost = v0[j + 1] + 1;
            const int insertionCost = v1[j] + 1;
            const int substitutionCost = v0[j] + (aCharLower == b[j].toLower() ? 0 : 1);
            v1[j + 1] = std::min(std::min(deletionCost, insertionCost), substitutionCost);
        }

        std::swap(v0, v1);
    }

    return v0[bSize];
}

qreal FuzzyCompare::jaroWinklerDistance(const QString &a, const QString &b)
{
    qreal m = 0;

    const int aSize = a.size();
    const int bSize = b.size();

    // Exit early if either are empty
    if (aSize == 0 || bSize == 0) {
        return 0;
    }

    // Exit early if they're an exact match.
    if (QString::compare(a, b, Qt::CaseInsensitive) == 0) {
        return 1;
    }

    const int range = (std::max(aSize, bSize) / 2) - 1;

    QVarLengthArray<bool, 64> aMatches(aSize, false);
    QVarLengthArray<bool, 64> bMatches(bSize, false);

    for (int i = 0; i < aSize; i++) {

        const int low = std::max(0, i - range);
        const int high = std::min(bSize - 1, i + range);

        for (int j = low; j <= high; j++) {
            if (!aMatches[i] && !bMatches[j] && a[i].toLower() == b[j].toLower()) {
                m += 1;
                aMatches[i] = true;
                bMatches[j] = true;
                break;
            }
        }
    }

    // Exit early if no matches were found
    if (m == 0) {
        return 0;
    }

    // Count the transpositions.
    int k = 0;
    int numTrans = 0;

    for (int i = 0; i < aSize; i++) {
        if (aMatches[i]) {
            int j = k;
            for (; j < bSize; j++) {
                if (bMatches[j]) {
                    k = j + 1;
                    break;
                }
            }

            if (j < bSize && a[i].toLower() != b[j].toLower()) {
                numTrans += 1;
            }
        }
    }

    qreal weight = (m / aSize + m / bSize + (m - (numTrans / 2.0)) / m) / 3;
    qreal l = 0;
    qreal p = 0.1;
    if (weight > 0.7) {
        while (l < aSize && l < bSize && a[l].toLower() == b[l].toLower() && l < 4) {
            l += 1;
        }

        weight += l * p * (1 - weight);
    }
    return weight;
}
