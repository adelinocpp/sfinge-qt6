#include "file_handler.h"
#include <QFileInfo>

namespace SFinGe {

bool FileHandler::saveImage(const QImage& image, const QString& filePath, int quality) {
    if (image.isNull()) {
        return false;
    }
    
    QString format = getFileExtension(filePath).toUpper();
    
    return image.save(filePath, format.toUtf8().constData(), quality);
}

QImage FileHandler::loadImage(const QString& filePath) {
    QImage image;
    image.load(filePath);
    return image;
}

QString FileHandler::getFileExtension(const QString& filePath) {
    QFileInfo fileInfo(filePath);
    return fileInfo.suffix();
}

bool FileHandler::fileExists(const QString& filePath) {
    QFileInfo fileInfo(filePath);
    return fileInfo.exists() && fileInfo.isFile();
}

}
