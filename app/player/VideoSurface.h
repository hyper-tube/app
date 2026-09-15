#pragma once

#include <QQuickFramebufferObject>
#include <QtQml/qqmlregistration.h>

namespace player {

class VideoSurface : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit VideoSurface(QQuickItem *parent = nullptr);
    ~VideoSurface() override;

    Renderer *createRenderer() const override;

protected:
    void itemChange(ItemChange change, const ItemChangeData &data) override;

private:
    void updateHold();
    void publishSupport() const;
    void watchWindow();

    QMetaObject::Connection m_windowWatch;
    bool m_holding = false;
};

}
