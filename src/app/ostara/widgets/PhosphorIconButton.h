// Stub — will be replaced with SVG icons
#ifndef PHOSPHORICONBUTTON_H
#define PHOSPHORICONBUTTON_H
#include <QPushButton>
class PhosphorIconButton : public QPushButton
{
public:
    explicit PhosphorIconButton(const QString &iconName, QWidget *parent = nullptr) : QPushButton(parent) { Q_UNUSED(iconName); setText(QStringLiteral("[icon]")); }
    void setHoverStyle(int style) { Q_UNUSED(style); }
};
#endif
