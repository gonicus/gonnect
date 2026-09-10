#pragma once

#include <QObject>

class HelpersTest : public QObject
{
    Q_OBJECT
public:
    explicit HelpersTest(QObject *parent = nullptr);

private slots:
    void initTestCase();

    void testFormatFileSize();
    void testFormatFileSizeClampsToPetabyte();

    void testGenerateSecretLength();
    void testGenerateSecretCharset();
    void testGenerateSecretIsRandom();
};
