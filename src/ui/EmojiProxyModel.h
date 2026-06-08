#pragma once

#include <QSortFilterProxyModel>
#include <QObject>
#include <QQmlEngine>

class EmojiProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")

    Q_PROPERTY(int group MEMBER m_group NOTIFY groupChanged FINAL)
    Q_PROPERTY(QString filterText MEMBER m_filterText NOTIFY filterTextChanged FINAL)

public:
    explicit EmojiProxyModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    int m_group = -1;
    QString m_filterText;

Q_SIGNALS:
    void groupChanged();
    void filterTextChanged();
};
