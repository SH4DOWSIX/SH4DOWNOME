#pragma once
#include <QPixmap>
#include <QString>

QPixmap svgToPixmap(const QString& svgPath, QSize boxSize);

// For composite SVG assembly in NoteAssembler
QString svgViewBox(const QString& svgPath);
QString svgInnerContent(const QString& svgPath); // inner SVG with currentColor replaced by white