#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QStringList>
#include <QPixmap>
#include <QSize>
#include <QVector>
#include <QTimer>
#include "subdivisionpattern.h"

class MetronomeEngine;

// ─── CustomPatternEditor ─────────────────────────────────────────────────────
// QObject model for the custom subdivision pattern editor bottom sheet.
// Owns all editor state; exposes properties and invokables to QML.
class CustomPatternEditor : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantList   pulses             READ pulses             NOTIFY pulsesChanged)
    Q_PROPERTY(int            pulseRevision      READ pulseRevision      NOTIFY pulsesChanged)
    Q_PROPERTY(int            selectedPulseIndex READ selectedPulseIndex NOTIFY selectionChanged)
    Q_PROPERTY(int            tileIndex          READ tileIndex          NOTIFY selectionChanged)
    Q_PROPERTY(int            tuplet             READ tuplet             NOTIFY selectionChanged)
    Q_PROPERTY(bool           dottedActive       READ dottedActive       NOTIFY selectionChanged)
    Q_PROPERTY(bool           isRest             READ isRest             NOTIFY selectionChanged)
    Q_PROPERTY(bool           accent             READ accent             NOTIFY selectionChanged)
    Q_PROPERTY(bool           canDot             READ canDot             NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList   tupletVisibility   READ tupletVisibility   NOTIFY selectionChanged)
    Q_PROPERTY(QString        totalDuration      READ totalDuration      NOTIFY pulsesChanged)
    Q_PROPERTY(QString        totalDurationColor READ totalDurationColor NOTIFY pulsesChanged)
    Q_PROPERTY(bool           canAccept          READ canAccept          NOTIFY pulsesChanged)
    Q_PROPERTY(QString        patternName        READ patternName  WRITE setPatternName NOTIFY patternNameChanged)
    Q_PROPERTY(bool           isPreviewing       READ isPreviewing       NOTIFY isPreviewingChanged)
    Q_PROPERTY(QStringList    presetNames        READ presetNames        CONSTANT)

public:
    explicit CustomPatternEditor(MetronomeEngine* engine, QObject* parent = nullptr);

    // Called by MetronomeController before opening the sheet
    void beginNew(int numerator, int denominator, bool compound);
    void beginEdit(const SubdivisionPattern& pattern, int numerator, int denominator, bool compound);

    SubdivisionPattern currentPattern() const;

    // ── Property getters ──────────────────────────────────────────────────
    QVariantList pulses()             const;
    int  pulseRevision()              const { return m_rev; }
    int  selectedPulseIndex()         const { return m_selectedIdx; }
    int  tileIndex()                  const { return m_tileIdx; }
    int  tuplet()                     const { return m_tuplet; }
    bool dottedActive()               const { return m_dotted; }
    bool isRest()                     const { return m_isRest; }
    bool accent()                     const { return m_accent; }
    bool canDot()                     const;
    QVariantList tupletVisibility()   const;
    QString totalDuration()           const { return m_totalText; }
    QString totalDurationColor()      const { return m_totalColor; }
    bool canAccept()                  const { return !m_pattern.pulses.isEmpty(); }
    QString patternName()             const { return m_name; }
    void setPatternName(const QString& n);
    bool isPreviewing()               const { return m_isPreviewing; }
    QStringList presetNames()         const;

    // ── QML invokables ────────────────────────────────────────────────────
    Q_INVOKABLE void selectPulse(int idx);
    Q_INVOKABLE void setTile(int tileIdx);
    Q_INVOKABLE void setTuplet(int n);
    Q_INVOKABLE void setDotted(bool d);
    Q_INVOKABLE void setIsRest(bool r);
    Q_INVOKABLE void setAccent(bool a);
    Q_INVOKABLE void addPulse();
    Q_INVOKABLE void removeSelected();
    Q_INVOKABLE void clearAll();
    Q_INVOKABLE void applyPreset(int presetIdx);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void movePulse(int from, int to);
    Q_INVOKABLE int  tileCount()       const;
    Q_INVOKABLE QString tileName(int idx) const;
    Q_INVOKABLE void startPreview();
    Q_INVOKABLE void stopPreview();

    // ── Called by NoteImageProvider ───────────────────────────────────────
    QPixmap tilePixmap(int tileIdx, QSize size) const;
    QPixmap pulsePixmap(int pulseIdx, QSize size) const;
    QPixmap previewPixmap(QSize size) const;

signals:
    void pulsesChanged();
    void selectionChanged();
    void patternNameChanged();
    void isPreviewingChanged();

private:
    void recomputeTotal();
    void pushUndo();
    NoteValue resolveActive() const;
    void syncControlsToSelectedPulse();
    bool isValidTupletForTile(int tileIdx, int tupletN) const;

    MetronomeEngine*   m_engine;
    SubdivisionPattern m_pattern;
    SubdivisionPattern m_savedPreviewPattern;
    int  m_numerator = 4, m_denominator = 4;
    bool m_compound  = false;

    int  m_rev        = 0;
    int  m_selectedIdx = -1;
    int  m_tileIdx    = 4;   // Quarter
    int  m_tuplet     = 0;
    bool m_dotted     = false;
    bool m_isRest     = false;
    bool m_accent     = false;

    QString m_name       = "Custom Pattern";
    QString m_totalText;
    QString m_totalColor;

    bool   m_isPreviewing = false;
    QTimer* m_previewTimer = nullptr;

    QVector<QVector<SubdivisionPulse>> m_undoStack;
};
