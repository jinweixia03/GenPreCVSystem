#include "errordialog.h"
#include "exceptions.h"
#include <QMessageBox>
#include <QWidget>

namespace GenPreCVSystem {
namespace Utils {

void ErrorDialog::showCritical(QWidget *parent, const QString &title, const QString &message)
{
    QMessageBox::critical(parent, title, message);
}

void ErrorDialog::showWarning(QWidget *parent, const QString &title, const QString &message)
{
    QMessageBox::warning(parent, title, message);
}

void ErrorDialog::showInformation(QWidget *parent, const QString &title, const QString &message)
{
    QMessageBox::information(parent, title, message);
}

void ErrorDialog::showException(QWidget *parent, const AppException &e)
{
    QMessageBox msgBox(parent);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setWindowTitle(QObject::tr("应用程序错误"));
    msgBox.setText(e.message());

    QString details = QObject::tr("时间: %1\n上下文: %2")
        .arg(e.timestamp().toString("yyyy-MM-dd hh:mm:ss"))
        .arg(e.context().isEmpty() ? "-" : e.context());

    msgBox.setDetailedText(details);
    msgBox.exec();
}

bool ErrorDialog::showConfirm(QWidget *parent, const QString &title, const QString &message)
{
    return QMessageBox::question(parent, title, message,
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No) == QMessageBox::Yes;
}

void ErrorDialog::showCriticalWithDetails(QWidget *parent, const QString &title,
                                           const QString &message, const QString &details)
{
    QMessageBox msgBox(parent);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setDetailedText(details);
    msgBox.exec();
}

} // namespace Utils
} // namespace GenPreCVSystem
