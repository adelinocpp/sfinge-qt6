#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <QString>
#include <QImage>

namespace SFinGe {

class FileHandler {
public:
    static bool saveImage(const QImage& image, const QString& filePath, int quality = -1);
    static QImage loadImage(const QString& filePath);
    
    static QString getFileExtension(const QString& filePath);
    static bool fileExists(const QString& filePath);
};

}

#endif
