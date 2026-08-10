// Stub — will be replaced with SVG icons
#ifndef ODOCKTITLEBAR_H
#define ODOCKTITLEBAR_H

#include <QWidget>

class QLabel;
class QPushButton;

class ODockTitleBar : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool collapsed READ isCollapsed WRITE setCollapsed)

public:
    explicit ODockTitleBar(const QString &title, QWidget *parent = nullptr);

    void setCollapsed(bool collapsed);
    bool isCollapsed() const { return m_collapsed; }

    void setFloating(bool floating);

    void setTitle(const QString &title);

signals:
    void collapseToggled(bool collapsed);
    void detachRequested();
    void closeRequested();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void updateCollapseIcon();

    QLabel *m_titleLabel = nullptr;
    QPushButton *m_collapseBtn = nullptr;
    QPushButton *m_detachBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
    bool m_collapsed = false;
};

#endif // ODOCKTITLEBAR_H
