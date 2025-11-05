#pragma once

#include <QMetaObject>
#include <QPointer>
#include <QQuickFramebufferObject>
#include <QString>

class TerminalBridge;

class PlainTextSurface : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *terminal READ terminal WRITE setTerminal NOTIFY terminalChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY fontFamilyChanged)
    Q_PROPERTY(qreal fontPointSize READ fontPointSize WRITE setFontPointSize NOTIFY fontPointSizeChanged)

public:
    explicit PlainTextSurface(QQuickItem *parent = nullptr);

    Renderer *createRenderer() const override;

    QObject *terminal() const;
    void setTerminal(QObject *terminal);

    QString fontFamily() const;
    void setFontFamily(const QString &family);

    qreal fontPointSize() const;
    void setFontPointSize(qreal pointSize);

signals:
    void terminalChanged();
    void fontFamilyChanged();
    void fontPointSizeChanged();

private:
    QPointer<TerminalBridge> m_terminal;
    QMetaObject::Connection m_bufferConnection;
    QString m_fontFamily = QStringLiteral("monospace");
    qreal m_fontPointSize = 13.0;
};
