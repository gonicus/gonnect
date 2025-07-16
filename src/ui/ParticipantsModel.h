#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

class JitsiConnector;

class ParticipantsModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(JitsiConnector *jitsiConnector MEMBER m_jitsiConnector NOTIFY jitsiConnectorChanged
                       FINAL)

public:
    enum class Roles { Id = Qt::UserRole + 1, DisplayName, Role };

    explicit ParticipantsModel(QObject *parent = nullptr);

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

private slots:
    void onJitsiConnectorChanged();

private:
    JitsiConnector *m_jitsiConnector = nullptr;
    QObject *m_jistiConnectorContext = nullptr;

signals:
    void jitsiConnectorChanged();
};
