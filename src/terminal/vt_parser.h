#ifndef TERMINAL_VT_PARSER_H
#define TERMINAL_VT_PARSER_H

#include "screen_buffer.h"

#include <QByteArray>
#include <QVector>

namespace terminal
{

enum class ParserState {
    Ground,
    Escape,
    CsiEntry,
    CsiParam,
    CsiIntermediate,
    OscString,
    SosPmApcString,
    DcsEntry,
    DcsParam,
    DcsIntermediate,
    DcsPassthrough,
    IgnoreUntilGround
};

class VtParser
{
public:
    explicit VtParser(ScreenBuffer &primaryScreen,
                      ScreenBuffer &alternateScreen);

    ~VtParser() = default;
    VtParser(const VtParser&) = delete;
    VtParser& operator=(const VtParser&) = delete;
    VtParser(VtParser&&) = default;
    VtParser& operator=(VtParser&&) = default;

    void reset();
    void feed(const QByteArray &data);
    void feed(const char *data, int length);

    ParserState state() const { return m_state; }

private:
    void handleGround(char byte);
    void handleEscape(char byte);
    void handleCsi(char byte);
    void handleOsc(char byte);
    void handleDcs(char byte);
    void flushUtf8Buffer();

    void executeControl(char byte);
    void collectParam(char byte);
    void collectIntermediate(char byte);
    void collectOsc(char byte);
    void collectDcs(char byte);
    void dispatchCsi();
    void dispatchOsc();
    void dispatchDcs();

    std::reference_wrapper<ScreenBuffer> m_primary;
    std::reference_wrapper<ScreenBuffer> m_alternate;
    std::reference_wrapper<ScreenBuffer> m_active;

    ParserState m_state = ParserState::Ground;

    QVector<int> m_params;
    QByteArray m_intermediates;
    QByteArray m_oscData;
    QByteArray m_dcsData;
    QByteArray m_utf8Buffer;

    int m_savedRow = 0;
    int m_savedColumn = 0;
    bool m_originMode = false;
    bool m_autoWrap = true;
    bool m_insertMode = false;
    bool m_bracketedPaste = false;
    bool m_screenAlternate = false;
};

}
#endif
