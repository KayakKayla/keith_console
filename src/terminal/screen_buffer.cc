#include "screen_buffer.h"

#include <QtGlobal>

namespace terminal
{

namespace {

constexpr char32_t kSpace = U' ';

CellAttributes defaultAttributes()
{
    return {};
}

Cell makeEmptyCell()
{
    Cell cell;
    cell.glyphs.append(kSpace);
    cell.attributes = defaultAttributes();
    cell.isDirty = true;
    return cell;
}

}

ScreenBuffer::ScreenBuffer(int rows, int columns)
    : m_rows(rows)
    , m_columns(columns)
    , m_cells(rows * columns, makeEmptyCell())
    , m_marginTop(0)
    , m_marginBottom(rows - 1)
{
}

void ScreenBuffer::moveCursor(int row, int column)
{
    m_cursorRow = qBound(0, row, m_rows - 1);
    m_cursorColumn = qBound(0, column, m_columns - 1);
}

void ScreenBuffer::carriageReturn()
{
    m_cursorColumn = 0;
}

void ScreenBuffer::lineFeed(bool allowScroll)
{
    if (m_cursorRow == m_marginBottom) {
        if (allowScroll) {
            scrollUp(1);
        }
    } else {
        m_cursorRow = qMin(m_cursorRow + 1, m_rows - 1);
    }
}

void ScreenBuffer::setMargin(int top, int bottom)
{
    m_marginTop = qBound(0, top, m_rows - 1);
    m_marginBottom = qBound(0, bottom, m_rows - 1);
    if (m_marginTop > m_marginBottom) {
        m_marginTop = 0;
        m_marginBottom = m_rows - 1;
    }
}

void ScreenBuffer::clear()
{
    for (int row = 0; row < m_rows; ++row) {
        clearRow(row);
    }
    moveCursor(0, 0);
}

void ScreenBuffer::clearRow(int row)
{
    if (row < 0 || row >= m_rows) {
        return;
    }

    for (int column = 0; column < m_columns; ++column) {
        const int index = (row * m_columns) + column;
        m_cells[index] = makeEmptyCell();
    }
    markRowDirty(row);
}

void ScreenBuffer::clearFromCursor()
{
    for (int column = m_cursorColumn; column < m_columns; ++column) {
        const int index = (m_cursorRow * m_columns) + column;
        m_cells[index] = makeEmptyCell();
    }
    markRowDirty(m_cursorRow);
}

void ScreenBuffer::clearToCursor()
{
    for (int column = 0; column <= m_cursorColumn; ++column) {
        const int index = (m_cursorRow * m_columns) + column;
        m_cells[index] = makeEmptyCell();
    }
    markRowDirty(m_cursorRow);
}

void ScreenBuffer::writeGlyph(char32_t codepoint, const CellAttributes &attributes)
{
    const int index = (m_cursorRow * m_columns) + m_cursorColumn;
    if (index < 0 || index >= m_cells.size()) {
        return;
    }

    Cell &cell = m_cells[index];
    cell.glyphs.clear();
    cell.glyphs.append(codepoint);
    cell.attributes = attributes;
    cell.isDirty = true;
    markRowDirty(m_cursorRow);

    m_cursorColumn += 1;
    wrapCursor();
}

void ScreenBuffer::writeText(const QString &text, const CellAttributes &attributes)
{
    for (const QChar &character : text) {
        writeGlyph(character.unicode(), attributes);
    }
}

void ScreenBuffer::scrollUp(int lines)
{
    if (lines <= 0) {
        return;
    }

    const int regionHeight = (m_marginBottom - m_marginTop) + 1;
    const int clampedLines = qMin(lines, regionHeight);
    if (clampedLines == regionHeight) {
        for (int row = m_marginTop; row <= m_marginBottom; ++row) {
            clearRow(row);
        }
        return;
    }

    for (int row = m_marginTop; row <= m_marginBottom - clampedLines; ++row) {
        const int sourceRow = row + clampedLines;
        for (int column = 0; column < m_columns; ++column) {
            const int destIndex = (row * m_columns) + column;
            const int srcIndex = (sourceRow * m_columns) + column;
            m_cells[destIndex] = m_cells[srcIndex];
            m_cells[destIndex].isDirty = true;
        }
        markRowDirty(row);
    }

    for (int row = m_marginBottom - clampedLines + 1; row <= m_marginBottom; ++row) {
        clearRow(row);
    }
}

void ScreenBuffer::scrollDown(int lines)
{
    if (lines <= 0) {
        return;
    }

    const int regionHeight = (m_marginBottom - m_marginTop) + 1;
    const int clampedLines = qMin(lines, regionHeight);
    if (clampedLines == regionHeight) {
        for (int row = m_marginTop; row <= m_marginBottom; ++row) {
            clearRow(row);
        }
        return;
    }

    for (int row = m_marginBottom; row >= m_marginTop + clampedLines; --row) {
        const int sourceRow = row - clampedLines;
        for (int column = 0; column < m_columns; ++column) {
            const int destIndex = (row * m_columns) + column;
            const int srcIndex = (sourceRow * m_columns) + column;
            m_cells[destIndex] = m_cells[srcIndex];
            m_cells[destIndex].isDirty = true;
        }
        markRowDirty(row);
    }

    for (int row = m_marginTop; row < m_marginTop + clampedLines; ++row) {
        clearRow(row);
    }
}

const QVector<Cell> &ScreenBuffer::rowData(int row) const
{
    Q_ASSERT(row >= 0 && row < m_rows);
    return *reinterpret_cast<const QVector<Cell> const *>(&m_cells) + (row * m_columns);
}

void ScreenBuffer::resetDirty()
{
    for (int row : std::as_const(m_dirtyRows)) {
        for (int column = 0; column < m_columns; ++column) {
            const int index = (row * m_columns) + column;
            m_cells[index].isDirty = false;
        }
    }
    m_dirtyRows.clear();
}

void ScreenBuffer::markRowDirty(int row)
{
    if (!m_dirtyRows.contains(row)) {
        m_dirtyRows.append(row);
    }
}

void ScreenBuffer::wrapCursor()
{
    if (m_cursorColumn < m_columns) {
        return;
    }
    m_cursorColumn = 0;
    if (m_cursorRow == m_marginBottom) {
        scrollUp(1);
    } else {
        m_cursorRow = qMin(m_cursorRow + 1, m_rows - 1);
    }
}

}
