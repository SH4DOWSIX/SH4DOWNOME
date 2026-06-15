#include "androidinputdialog.h"
#include <QMetaObject>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QJniEnvironment>
#include <jni.h>
#endif

static AndroidInputDialog *s_instance = nullptr;

// ── JNI callbacks (called from Android UI thread) ──────────────────────────
#ifdef Q_OS_ANDROID
static void jniAccepted(JNIEnv *env, jclass, jstring text)
{
    const char *chars = env->GetStringUTFChars(text, nullptr);
    QString qtext = QString::fromUtf8(chars);
    env->ReleaseStringUTFChars(text, chars);
    if (s_instance)
        QMetaObject::invokeMethod(s_instance, [qtext]() { s_instance->emitAccepted(qtext); });
}

static void jniCancelled(JNIEnv *, jclass)
{
    if (s_instance)
        QMetaObject::invokeMethod(s_instance, []() { s_instance->emitCancelled(); });
}
#endif

// ── Constructor ─────────────────────────────────────────────────────────────
AndroidInputDialog::AndroidInputDialog(QObject *parent) : QObject(parent)
{
    s_instance = this;

#ifdef Q_OS_ANDROID
    static const JNINativeMethod methods[] = {
        { "nativeAccepted", "(Ljava/lang/String;)V", reinterpret_cast<void *>(jniAccepted) },
        { "nativeCancelled", "()V",                  reinterpret_cast<void *>(jniCancelled) }
    };
    QJniEnvironment env;
    env.registerNativeMethods(
        "org/qtproject/example/SH4DOWNOME/InputDialog",
        methods, std::size(methods));
#endif
}

// ── Emit helpers (always on Qt main thread) ─────────────────────────────────
void AndroidInputDialog::emitAccepted(const QString &text) { emit accepted(text); }
void AndroidInputDialog::emitCancelled()                   { emit cancelled();    }

// ── Public API ───────────────────────────────────────────────────────────────
void AndroidInputDialog::showText(const QString &title, const QString &defaultText)
{
#ifdef Q_OS_ANDROID
    jint accentArgb = (jint)m_accentColor.rgba();
    QJniObject activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative", "activity", "()Landroid/app/Activity;");
    QJniObject::callStaticMethod<void>(
        "org/qtproject/example/SH4DOWNOME/InputDialog", "show",
        "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;ZI)V",
        activity.object(),
        QJniObject::fromString(title).object(),
        QJniObject::fromString(defaultText).object(),
        jboolean(false),
        accentArgb);
#else
    Q_UNUSED(title); Q_UNUSED(defaultText);
#endif
}

void AndroidInputDialog::showNumber(const QString &title, int value)
{
#ifdef Q_OS_ANDROID
    jint accentArgb = (jint)m_accentColor.rgba();
    QJniObject activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative", "activity", "()Landroid/app/Activity;");
    QJniObject::callStaticMethod<void>(
        "org/qtproject/example/SH4DOWNOME/InputDialog", "show",
        "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;ZI)V",
        activity.object(),
        QJniObject::fromString(title).object(),
        QJniObject::fromString(QString::number(value)).object(),
        jboolean(true),
        accentArgb);
#else
    Q_UNUSED(title); Q_UNUSED(value);
#endif
}

void AndroidInputDialog::showBulkAdd(int fromTempo)
{
#ifdef Q_OS_ANDROID
    jint accentArgb = (jint)m_accentColor.rgba();
    QJniObject activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative", "activity", "()Landroid/app/Activity;");
    QJniObject::callStaticMethod<void>(
        "org/qtproject/example/SH4DOWNOME/InputDialog", "showBulkAdd",
        "(Landroid/app/Activity;II)V",
        activity.object(),
        jint(fromTempo),
        accentArgb);
#else
    Q_UNUSED(fromTempo);
#endif
}

void AndroidInputDialog::showImeForFocused()
{
#ifdef Q_OS_ANDROID
    QJniObject activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative", "activity", "()Landroid/app/Activity;");
    QJniObject::callStaticMethod<void>(
        "org/qtproject/example/SH4DOWNOME/InputDialog", "showImeForFocused",
        "(Landroid/app/Activity;)V",
        activity.object());
#endif
}
