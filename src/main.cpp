#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QFile>
#include <QPalette>
#include <QStyleFactory>
#include "MetronomeController.h"
#include "BeatIndicatorItem.h"
#include "NoteImageProvider.h"
#include "SectionListModel.h"
#include "androidinputdialog.h"

int main(int argc, char *argv[])
{
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);

    // Fusion widget style (used by the SubdivisionSelectorDialog which remains a QWidget)
#ifndef Q_OS_ANDROID
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window,          QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText,       Qt::white);
    darkPalette.setColor(QPalette::Base,             QColor(42, 42, 42));
    darkPalette.setColor(QPalette::AlternateBase,    QColor(66, 66, 66));
    darkPalette.setColor(QPalette::ToolTipBase,      QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipText,      Qt::white);
    darkPalette.setColor(QPalette::Text,             Qt::white);
    darkPalette.setColor(QPalette::Button,           QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText,       Qt::white);
    darkPalette.setColor(QPalette::BrightText,       Qt::red);
    darkPalette.setColor(QPalette::Highlight,        QColor(150, 0, 0));
    darkPalette.setColor(QPalette::HighlightedText,  Qt::white);
    app.setPalette(darkPalette);

    // Apply dark.qss to widget dialogs (SubdivisionSelectorDialog etc.)
    QFile f(":/themes/dark.qss");
    if (f.open(QFile::ReadOnly | QFile::Text))
        app.setStyleSheet(f.readAll());
#endif

    // Quick Controls 2 style – use Fusion (available in Qt6 QuickControls2)
    QQuickStyle::setStyle("Fusion");

    // Register QML types
    qmlRegisterType<BeatIndicatorItem>("com.sh4downome", 1, 0, "BeatIndicatorItem");
    qmlRegisterUncreatableType<SectionListModel>("com.sh4downome", 1, 0, "SectionListModel",
                                                  "Access via controller.sectionModel");

    // Create the controller (owns the engine, preset manager, etc.)
    MetronomeController controller;
    AndroidInputDialog androidInput;

    QQmlApplicationEngine engine;

    // Expose controller as a context property accessible from all QML files
    engine.rootContext()->setContextProperty("controller", &controller);
    engine.rootContext()->setContextProperty("androidInput", &androidInput);
    engine.rootContext()->setContextProperty("appVersion", QStringLiteral(APP_VERSION));

    // Register the note image provider (engine takes ownership)
    engine.addImageProvider("notes", new NoteImageProvider(&controller));

    // Load the main QML file from the QRC
    const QUrl url(QStringLiteral("qrc:/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject* obj, const QUrl& objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
