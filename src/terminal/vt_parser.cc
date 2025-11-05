#include "vt_parser.h"

#include <QtGlobal>

namespace terminal
{

namespace {

bool isIntermediate(char byte)
{
    return byte >= 0x20 && byte <= 0x2F;
}

bool isParameter(char byte)
{
    return byte >= 0x30 && byte <= 0x3F;
}

bool isUtf8Start(char byte)
{
    return static_cast<unsigned char>(byte) >= 0xC2;
}

}

VtParser::VtParser(ScreenBuffer &primaryScreen, ScreenBuffer &alternateScreen)
    : m_primary(primaryScreen)
    , m_alternate(alternateScreen)
{
    reset();
}

void VtParser::reset()
{
    m_state = ParserState::Ground;
    m_params.clear();
    m_intermediates.clear();
    m_oscData.clear();
    m_dcsData.clear();
    m_utf8Buffer.clear();
    m_savedRow = 0;
    m_savedColumn = 0;
    m_originMode = false;
    m_autoWrap = true;
    m_insertMode = false;
    m_bracketedPaste = false;
    m_useAlternateScreen = false;
}

void VtParser::feed(const QByteArray &data)
{
    feed(data.constData(), data.size());
}

void VtParser::feed(const char *data, int length)
{
    for (int i = 0; i < length; ++i) {
        const char byte = data[i];
        switch (m_state) {
        case ParserState::Ground:
            handleGround(byte);
            break;
        case ParserState::Escape:
            handleEscape(byte);
            break;
        case ParserState::CsiEntry:
        case ParserState::CsiParam:
        case ParserState::CsiIntermediate:
            handleCsi(byte);
            break;
        case ParserState::OscString:
            handleOsc(byte);
            break;
        case ParserState::SosPmApcString:
            if (byte == 0x07 || byte == 0x9C) {
                m_state = ParserState::Ground;
            }
            break;
        case ParserState::DcsEntry:
        case ParserState::DcsParam:
        case ParserState::DcsIntermediate:
        case ParserState::DcsPassthrough:
            handleDcs(byte);
            break;
        case ParserState::IgnoreUntilGround:
            if (byte == 0x1B || byte == 0x9C) {
                m_state = ParserState::Ground;
            }
            break;
        }
    }
}

void VtParser::handleGround(char byte)
{
    if (byte == 0x1B) { // ESC
        flushUtf8Buffer();
        m_state = ParserState::Escape;
        return;
    }

    if (byte <= 0x1F || byte == 0x7F) {
        flushUtf8Buffer();
        executeControl(byte);
        return;
    }

    if (isUtf8Start(byte) || !m_utf8Buffer.isEmpty()) {
        m_utf8Buffer.append(byte);
        if (QByteArray::fromRawData(m_utf8Buffer.constData(), m_utf8Buffer.size()).isEmpty()) {
            return;
        }
    } else {
        m_utf8Buffer.append(byte);
    }

    flushUtf8Buffer();
}

void VtParser::handleEscape(char byte)
{
    if (byte == '[') {
        m_params.clear();
        m_intermediates.clear();
        m_state = ParserState::CsiEntry;
        return;
    }

    if (byte == ']') {
        m_oscData.clear();
        m_state = ParserState::OscString;
        return;
    }

    if (byte == 'P') {
        m_dcsData.clear();
        m_state = ParserState::DcsEntry;
        return;
    }

    if (byte == 'X' || byte == '^' || byte == '_') {
        m_state = ParserState::SosPmApcString;
        return;
    }

    executeControl(byte);
    m_state = ParserState::Ground;
}

void VtParser::handleCsi(char byte)
{
    if (m_state == ParserState::CsiEntry) {
        if (isParameter(byte)) {
            m_state = ParserState::CsiParam;
            collectParam(byte);
            return;
        }
        if (isIntermediate(byte)) {
            m_state = ParserState::CsiIntermediate;
            collectIntermediate(byte);
            return;
        }
    }

    if (m_state == ParserState::CsiParam) {
        if (isParameter(byte)) {
            collectParam(byte);
            return;
        }
        if (isIntermediate(byte)) {
            m_state = ParserState::CsiIntermediate;
            collectIntermediate(byte);
            return;
        }
    }

    if (m_state == ParserState::CsiIntermediate) {
        if (isIntermediate(byte)) {
            collectIntermediate(byte);
            return;
        }
    }

    if (byte >= 0x40 && byte <= 0x7E) {
        dispatchCsi();
        m_state = ParserState::Ground;
        return;
    }

    if (byte == 0x1B) {
        m_state = ParserState::Escape;
        return;
    }

    if (byte <= 0x1F) {
        executeControl(byte);
    }
}

void VtParser::handleOsc(char byte)
{
    if (byte == 0x07 || byte == 0x9C) {
        dispatchOsc();
        m_state = ParserState::Ground;
        return;
    }

    if (byte == 0x1B) {
        m_state = ParserState::Escape;
        return;
    }

    m_oscData.append(byte);
}

void VtParser::handleDcs(char byte)
{
    if (m_state == ParserState::DcsEntry) {
        m_state = ParserState::DcsParam;
    }

    if (m_state == ParserState::DcsParam) {
        if (isParameter(byte)) {
            collectDcs(byte);
            return;
        }
        if (isIntermediate(byte)) {
            m_state = ParserState::DcsIntermediate;
            collectIntermediate(byte);
            return;
        }
        m_state = ParserState::DcsPassthrough;
    }

    if (m_state == ParserState::DcsIntermediate) {
        if (isIntermediate(byte)) {
            collectIntermediate(byte);
            return;
        }
        m_state = ParserState::DcsPassthrough;
    }

    if (byte == 0x07 || byte == 0x9C) {
        dispatchDcs();
        m_state = ParserState::Ground;
        return;
    }

    if (byte == 0x1B) {
        m_state = ParserState::Escape;
        return;
    }

    if (byte <= 0x1F) {
        executeControl(byte);
        return;
    }

    if (m_state == ParserState::DcsPassthrough) {
        m_dcsData.append(byte);
    }
}

void VtParser::flushUtf8Buffer()
{
    if (m_utf8Buffer.isEmpty()) {
        return;
    }

    const QString text = QString::fromUtf8(m_utf8Buffer.constData(), m_utf8Buffer.size());
    m_utf8Buffer.clear();

    CellAttributes attrs;
    activeScreen().writeText(text, attrs);
}

void VtParser::executeControl(char byte)
{
    Q_UNUSED(byte);
    // TODO: Implement C0/C1 control handling (BEL, BS, HT, LF, CR, etc.).
}

void VtParser::collectParam(char byte)
{
    if (byte == ';') {
        if (m_params.isEmpty()) {
            m_params.append(0);
        }
        m_params.append(0);
        return;
    }

    if (m_params.isEmpty()) {
        m_params.append(0);
    }

    m_params.last() = (m_params.last() * 10) + (byte - '0');
}

void VtParser::collectIntermediate(char byte)
{
    m_intermediates.append(byte);
}

void VtParser::collectOsc(char byte)
{
    m_oscData.append(byte);
}

void VtParser::collectDcs(char byte)
{
    m_dcsData.append(byte);
}

void VtParser::dispatchCsi()
{
    // TODO: Interpret CSI sequences using m_params, m_intermediates, and the final byte.
}

void VtParser::dispatchOsc()
{
    // TODO: Interpret OSC sequences (title, clipboard, hyperlinks, etc.).
}

void VtParser::dispatchDcs()
{
    // TODO: Interpret DCS sequences (Sixel, DECRQSS, etc.).
}

ScreenBuffer &VtParser::activeScreen()
{
    return m_useAlternateScreen ? m_alternate : m_primary;
}

const ScreenBuffer &VtParser::activeScreen() const
{
    return m_useAlternateScreen ? m_alternate : m_primary;
}

} // namespace terminal
