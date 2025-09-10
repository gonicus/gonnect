#pragma once

#include <QApplication>
#include <QSocketNotifier>
#include <QQuickWindow>
#include <QObject>
#include <QTranslator>

#define RESTART_CODE 255

// The number of miliseconds a call shall be visible after it has ended
#define GONNECT_CALL_VISIBLE_AFTER_END 3500

class Application : public QApplication
{
    Q_OBJECT

public:
    explicit Application(int &argc, char **argv);
    ~Application();

    bool isFirstInstance() const;
    void sendArguments() const;

    void setRootWindow(QQuickWindow *win) { m_rootWindow = win; }
    QQuickWindow *rootWindow() const { return m_rootWindow; }

    // Unix signal handlers
    static void hupSignalHandler(int unused);
    static void termSignalHandler(int unused);

    bool isDebugRun() const { return m_isDebugRun; }

    static QString logFilePath();
    static QString logFileName();

    static bool isFlatpak();

public Q_SLOTS:
    void handleSigHup();
    void handleSigTerm();
    void shutdown();

private:
    static void fileMessageHandler(QtMsgType type, const QMessageLogContext &context,
                                   const QString &msg);

    void initDebugRun();
    void initialize();
    void initializeSIP();
    void installTranslations();

    static int s_sighupFd[2];
    static int s_sigtermFd[2];

    QQuickWindow *m_rootWindow = nullptr;

    QTranslator m_appTranslator;
    QTranslator m_baseTranslator;
    QTranslator m_declarativeTranslator;

    QSocketNotifier *m_hupNotifier = nullptr;
    QSocketNotifier *m_termNotifier = nullptr;

    bool m_initialized = false;
    bool m_isDebugRun = false;
};
