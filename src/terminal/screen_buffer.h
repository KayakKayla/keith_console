#ifndef TERMINAL_SCREEN_BUFFER_H
#define TERMINAL_SCREEN_BUFFER_H

#include <QVector>
#include <QString>

#include <cstdint>

namespace terminal
{

struct CellAttributes
{
    quint32 foreground = 0x00C0C0C0;
    quint32 background = 0x00101010;
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool inverse = false;
    bool blink = false;
    bool invisible = false;
};

struct Cell
{
    QVector<char32_t> glyphs;
    CellAttributes attributes;
    bool isDirty = true;
};

class ScreenBuffer
{
public:
    ScreenBuffer(int rows, int columns);

    int rows() const { return m_rows; }
    int columns() const { return m_columns; }

    void moveCursor(int row, int column);
    void carriageReturn();
    void lineFeed(bool allowScroll = true);
    void setMargin(int top, int bottom);

    void clear();
    void clearRow(int row);
    void clearFromCursor();
    void clearToCursor();
    void writeGlyph(char32_t codepoint, const CellAttributes &attributes);
    void writeText(const QString &text, const CellAttributes &attributes);

    void scrollUp(int lines = 1);
    void scrollDown(int lines = 1);

    const QVector<int> &dirtyRows() const { return m_dirtyRows; }
    const QVector<Cell> &rowData(int row) const;
    void resetDirty();

private:
    void markRowDirty(int row);
    void wrapCursor();

    int m_rows;
    int m_columns;
    QVector<Cell> m_cells;
    QVector<int> m_dirtyRows;

    int m_cursorRow = 0;
    int m_cursorColumn = 0;
    int m_marginTop = 0;
    int m_marginBottom;
};

}
#endif
