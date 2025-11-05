#include "plain_text_surface.h"

#include "terminal/terminal_bridge.h"

#include <QColor>
#include <QFont>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QStringList>
#include <QtMath>

namespace {
class PlainTextRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions
{
public:
    explicit PlainTextRenderer(const PlainTextSurface *surface)
        : m_surface(surface)
    {
        initializeOpenGLFunctions();
        m_font.setFamily("monospace");
        m_font.setPointSize(13);
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        if (size.width() <= 0 || size.height() <= 0) {
            return nullptr;
        }
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        m_paintDevice.setSize(size);
        return new QOpenGLFramebufferObject(size, format);
    }

    void render() override
    {
        auto *fbo = framebufferObject();
        if (!fbo) {
            return;
        }

        const QSize size = fbo->size();
        glViewport(0, 0, size.width(), size.height());
        glClearColor(0.07f, 0.07f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        m_paintDevice.setSize(size);

        QPainter painter(&m_paintDevice);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        painter.fillRect(QRect(QPoint(0, 0), size), QColor(16, 16, 16));
        painter.setPen(QColor(80, 240, 120));
        m_font.setFamily(m_surface->fontFamily());
        m_font.setPointSizeF(m_surface->fontPointSize());
        painter.setFont(m_font);

        const TerminalBridge *terminal = qobject_cast<TerminalBridge *>(m_surface->terminal());
        if (terminal) {
            const QString text = terminal->buffer();
            const QStringList lines = text.split('\n');
            const qreal lineHeight = m_surface->fontPointSize() + 4;
            qreal y = lineHeight;
            for (const QString &line : lines) {
                painter.drawText(QPointF(6, y), line);
                y += lineHeight;
                if (y > size.height() + lineHeight) {
                    break;
                }
            }
        }
        painter.end();

        update();
    }

private:
    const PlainTextSurface *m_surface;
    QOpenGLPaintDevice m_paintDevice;
    QFont m_font;
};
} // namespace

PlainTextSurface::PlainTextSurface(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
{
    setMirrorVertically(true);
}

QQuickFramebufferObject::Renderer *PlainTextSurface::createRenderer() const
{
    return new PlainTextRenderer(this);
}

QObject *PlainTextSurface::terminal() const
{
    return m_terminal;
}

void PlainTextSurface::setTerminal(QObject *terminal)
{
    if (m_terminal == terminal) {
        return;
    }

    if (m_terminal) {
        QObject::disconnect(m_bufferConnection);
    }

    m_terminal = qobject_cast<TerminalBridge *>(terminal);
    if (m_terminal) {
        m_bufferConnection = connect(m_terminal, &TerminalBridge::bufferChanged, this, &PlainTextSurface::update);
    }
    emit terminalChanged();
    update();
}

QString PlainTextSurface::fontFamily() const
{
    return m_fontFamily;
}

void PlainTextSurface::setFontFamily(const QString &family)
{
    if (m_fontFamily == family) {
        return;
    }
    m_fontFamily = family;
    emit fontFamilyChanged();
    update();
}

qreal PlainTextSurface::fontPointSize() const
{
    return m_fontPointSize;
}

void PlainTextSurface::setFontPointSize(qreal pointSize)
{
    if (qFuzzyCompare(m_fontPointSize, pointSize)) {
        return;
    }
    m_fontPointSize = pointSize;
    emit fontPointSizeChanged();
    update();
}
