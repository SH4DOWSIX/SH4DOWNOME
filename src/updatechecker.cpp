#include "updatechecker.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDesktopServices>
#include <QPointer>
#include <QPushButton>
#include <QSettings>
#include <QUrl>
#include <QString>
#include <QStringList>
#include <array>

#ifndef APP_VERSION
#define APP_VERSION "0.0.0"
#endif

static bool isNewerVersion(const QString& current, const QString& remote)
{
    auto parts = [](const QString& v) -> std::array<int, 3> {
        QStringList p = v.split('.');
        while (p.size() < 3) p.append("0");
        return {p[0].toInt(), p[1].toInt(), p[2].toInt()};
    };
    auto c = parts(current);
    auto r = parts(remote);
    for (int i = 0; i < 3; i++) {
        if (r[i] > c[i]) return true;
        if (r[i] < c[i]) return false;
    }
    return false;
}

void UpdateChecker::check(QWidget* parent, bool silent)
{
    auto* nam = new QNetworkAccessManager();

    QNetworkRequest req(QUrl("https://api.github.com/repos/SH4DOWSIX/SH4DOWNOME/releases/latest"));
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setRawHeader("User-Agent", "SH4DOWNOME-UpdateChecker");

    QNetworkReply* reply = nam->get(req);

    QPointer<QWidget> safeParent(parent);

    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        reply->deleteLater();
        nam->deleteLater();
        QWidget* dialogParent = safeParent ? safeParent.data() : nullptr;

        if (reply->error() != QNetworkReply::NoError) {
            if (!silent) {
                QMessageBox::warning(dialogParent, "Update Check",
                    "Could not check for updates:\n" + reply->errorString());
            }
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) return;

        QJsonObject obj = doc.object();
        QString tagName  = obj["tag_name"].toString();
        QString htmlUrl  = obj["html_url"].toString();

        // Strip leading 'v' from tag (e.g. "v3.0.4" -> "3.0.4")
        QString remoteVersion = tagName.startsWith('v') ? tagName.mid(1) : tagName;
        QString currentVersion = QString(APP_VERSION);

        if (isNewerVersion(currentVersion, remoteVersion)) {
            QSettings settings;
            const QString ignoredVersion = settings.value("updates/ignoredVersion").toString();
            if (silent && ignoredVersion == remoteVersion)
                return;

            QMessageBox msgBox(dialogParent);
            msgBox.setWindowTitle("Update Available");
            msgBox.setTextFormat(Qt::RichText);
            msgBox.setText(QString(
                "A new version of SH4DOWNOME is available: <b>%1</b><br>"
                "You are running <b>%2</b>.")
                .arg(remoteVersion, currentVersion));
            msgBox.setInformativeText("Open the download page?");
            QPushButton* openButton = msgBox.addButton("Open", QMessageBox::AcceptRole);
            QPushButton* laterButton = msgBox.addButton("Later", QMessageBox::RejectRole);
            QPushButton* ignoreButton = msgBox.addButton("Ignore This Version", QMessageBox::DestructiveRole);
            msgBox.setDefaultButton(openButton);

            msgBox.exec();

            if (msgBox.clickedButton() == openButton) {
                QDesktopServices::openUrl(QUrl(htmlUrl));
            } else if (msgBox.clickedButton() == ignoreButton) {
                settings.setValue("updates/ignoredVersion", remoteVersion);
            } else if (msgBox.clickedButton() == laterButton) {
                return;
            }
        } else if (!silent) {
            QMessageBox::information(dialogParent, "No Updates",
                QString("You are on the latest version (%1).").arg(currentVersion));
        }
    });
}
