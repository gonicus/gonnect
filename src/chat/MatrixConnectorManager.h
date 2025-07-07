#pragma once

#include <QObject>
#include <QQmlEngine>

class MatrixConnector;

class MatrixConnectorManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isMatrixAvailable READ isMatrixAvailable NOTIFY matrixConnectorsChanged FINAL)
    Q_PROPERTY(QList<MatrixConnector *> matrixConnectors READ matrixConnectors NOTIFY
                       matrixConnectorsChanged FINAL)

public:
    static MatrixConnectorManager &instance()
    {
        static MatrixConnectorManager *_instance = nullptr;
        if (!_instance) {
            _instance = new MatrixConnectorManager;
        }
        return *_instance;
    }

    bool isInitialized() const { return m_isInitialized; }
    bool isMatrixAvailable() const { return !m_connectors.isEmpty(); }
    QList<MatrixConnector *> matrixConnectors() const { return m_connectors; };

    void saveRecoveryKey(const QString &settingsGroup, const QString &key) const;
    void saveAccessToken(const QString &settingsGroup, const QString &token) const;

private slots:
    void init();

private:
    explicit MatrixConnectorManager(QObject *parent = nullptr);

    bool m_isInitialized = false;
    bool m_isMatrixAvailable = false;
    QList<MatrixConnector *> m_connectors;

signals:
    void matrixConnectorsChanged();
};

class MatrixConnectorManagerWrapper
{
    Q_GADGET
    QML_FOREIGN(MatrixConnectorManager)
    QML_NAMED_ELEMENT(MatrixConnectorManager)
    QML_SINGLETON

public:
    static MatrixConnectorManager *create(QQmlEngine *, QJSEngine *)
    {
        return &MatrixConnectorManager::instance();
    }

private:
    MatrixConnectorManagerWrapper() = default;
};
