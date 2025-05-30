#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QSortFilterProxyModel>

class HistoryProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

    Q_PROPERTY(QString filterText MEMBER m_filterText NOTIFY filterTextChanged FINAL)
    Q_PROPERTY(HistoryProxyModel::TypeFilter typeFilter MEMBER m_typeFilter NOTIFY typeFilterChanged
                       FINAL)

public:
    enum class TypeFilter { ALL, INCOMING, OUTGOING, MISSED };
    Q_ENUM(TypeFilter)

    explicit HistoryProxyModel(QObject *parent = nullptr);

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    TypeFilter m_typeFilter = TypeFilter::ALL;
    QString m_filterText;

signals:
    void filterTextChanged();
    void typeFilterChanged();
};
