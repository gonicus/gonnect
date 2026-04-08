#pragma once

#include <QSortFilterProxyModel>
#include <QQmlEngine>

class RTTProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool showOnlyFinished READ showOnlyFinished WRITE setShowOnlyFinished NOTIFY
                       showOnlyFinishedChanged)

public:
    explicit RTTProxyModel(QObject *parent = nullptr);

    bool showOnlyFinished() const;
    void setShowOnlyFinished(bool showFinished);

Q_SIGNALS:
    void showOnlyFinishedChanged();

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    bool m_showOnlyFinished = false;
};
