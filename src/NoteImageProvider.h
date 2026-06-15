#pragma once

#include <QQuickImageProvider>
#include <QPixmap>
#include <QSize>
#include <QString>

class MetronomeController;

class NoteImageProvider : public QQuickImageProvider {
public:
    explicit NoteImageProvider(MetronomeController* controller)
        : QQuickImageProvider(QQuickImageProvider::Pixmap)
        , m_controller(controller)
    {}

    // id format:
    //   "current_<revision>"          -> current section's subdivision icon
    //   "section_<idx>_<revision>"    -> specific section's subdivision icon
    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;

private:
    MetronomeController* m_controller;
};
