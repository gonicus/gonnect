#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

class SIPTemplate;

class SIPTemplateModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString templateId MEMBER m_templateId NOTIFY templateIdChanged FINAL)

public:
    enum class Roles {
        Name = Qt::UserRole + 1,
        Description,
        Preset,
        Target,
        Type,
        Regex,
        Required,
        MimeType,
        FileSuffixes,
    };

    explicit SIPTemplateModel(QObject *parent = nullptr);

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

private slots:
    void onTemplateIdChanged();

private:
    QString m_templateId;
    SIPTemplate *m_template = nullptr;

signals:
    void templateIdChanged();
};
