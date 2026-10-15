#pragma once

#include <QSortFilterProxyModel>
#include <QQmlEngine>

class ActivitiesProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")

    Q_PROPERTY(bool isFiltering READ isFiltering NOTIFY isFilteringChanged FINAL)
    Q_PROPERTY(QString filterText MEMBER m_filterText NOTIFY filterTextChanged FINAL)
    Q_PROPERTY(ActivitiesProxyModel::MediumFilter mediumFilter MEMBER m_mediumFilter NOTIFY
                       mediumFilterChanged FINAL)

public:
    enum class MediumFilter { ALL, SIPCALL, JITSIMEET, CHAT };
    Q_ENUM(MediumFilter)

    explicit ActivitiesProxyModel(QObject *parent = nullptr);

    bool isFiltering() const { return m_isFiltering; }

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    bool m_isFiltering = false;
    QString m_filterText;
    ActivitiesProxyModel::MediumFilter m_mediumFilter = ActivitiesProxyModel::MediumFilter::ALL;

private Q_SLOTS:
    void updateIsFiltering();

Q_SIGNALS:
    void filterTextChanged();
    void mediumFilterChanged();
    void isFilteringChanged();
};
