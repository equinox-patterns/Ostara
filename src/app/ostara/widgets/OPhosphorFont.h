// Stub — Phosphor font functionality will be replaced with SVG icons
#ifndef OPHOSPHORFONT_H
#define OPHOSPHORFONT_H
#include <QChar>
#include <QFont>
struct OPhosphorFont
{
    enum Style { Regular, Fill, Bold, Duotone };
    static auto font(Style style = Regular, int pixelSize = 16) -> QFont { Q_UNUSED(style); QFont f; f.setPixelSize(pixelSize); return f; }
    static auto codepoint(const QString &name) -> QChar { Q_UNUSED(name); return QChar(0x25A1); }
};
#endif
