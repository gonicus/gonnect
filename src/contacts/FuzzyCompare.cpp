#include <QRegularExpression>
#include "FuzzyCompare.h"

quint8 FuzzyCompare::levenshteinDistance(const QString &a, const QString &b)
{
    const auto n = b.size();
    QList<quint8> v0(n + 1);
    QList<quint8> v1(n + 1);

    for (int i = 0; i <= n; ++i) {
        v0[i] = i;
    }

    for (int i = 0; i < a.size(); ++i) {
        v1[0] = i + 1;

        for (int j = 0; j < n; ++j) {
            const quint8 deletionCost = v0[j + 1] + 1;
            const quint8 insertionCost = v1[j] + 1;
            quint8 substitutionCost = 0;

            if (a[i] == b[j]) {
                substitutionCost = v0[j];
            } else {
                substitutionCost = v0[j] + 1;
            }

            v1[j + 1] = std::min({ deletionCost, insertionCost, substitutionCost });
        }

        v0 = v1;
    }

    return v0[n];
}

qreal FuzzyCompare::jaroWinklerDistance(const QString &a, const QString &b)
{
    // const bool caseSensitive = false;

    float m = 0;
    int low, high, range;
    int k = 0, numTrans = 0;

    const auto aSize = a.size();
    const auto bSize = b.size();

    // Exit early if either are empty
    if (aSize == 0 || bSize == 0) {
        return 0;
    }

    // Convert to lower if case-sensitive is false
    // if (caseSensitive == false) {
    //     std::transform(a.begin(), a.end(), a.begin(), ::tolower);
    //     std::transform(b.begin(), b.end(), b.begin(), ::tolower);
    // }

    // Exit early if they're an exact match.
    if (a == b) {
        return 1;
    }

    range = (std::max(aSize, bSize) / 2) - 1;

    std::vector<int> aMatches(aSize);
    std::vector<int> bMatches(bSize);

    for (int i = 0; i < aSize; i++) {

        // Low Value;
        if (i >= range) {
            low = i - range;
        } else {
            low = 0;
        }

        // High Value;
        if (i + range <= (bSize - 1)) {
            high = i + range;
        } else {
            high = bSize - 1;
        }

        for (int j = low; j <= high; j++) {
            if (aMatches[i] != 1 && bMatches[j] != 1 && a[i] == b[j]) {
                m += 1;
                aMatches[i] = 1;
                bMatches[j] = 1;
                break;
            }
        }
    }

    // Exit early if no matches were found
    if (m == 0) {
        return 0;
    }

    // Count the transpositions.
    for (int i = 0; i < aSize; i++) {
        if (aMatches[i] == 1) {
            int j;
            for (j = k; j < bSize; j++) {
                if (bMatches[j] == 1) {
                    k = j + 1;
                    break;
                }
            }

            if (a[i] != b[j]) {
                numTrans += 1;
            }
        }
    }

    float weight = (m / aSize + m / bSize + (m - (numTrans / 2)) / m) / 3;
    float l = 0;
    float p = 0.1;
    if (weight > 0.7) {
        while (l < aSize && l < bSize && a[l] == b[l] && l < 4) {
            l += 1;
        }

        weight += l * p * (1 - weight);
    }
    return weight;
}
