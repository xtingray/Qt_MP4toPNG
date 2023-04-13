#include <QtCore>
#include "qcutter.h"

int main(int argc, char** argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    QString inputFile = "/tmp/input.mp4"; // Video input
    QString outputFolder = "/tmp/output"; // Folder where PNG sequence will be stored
    int framesTotal = 10; // Number of PNG files to be created

    QCutter *cutter = new QCutter();
    cutter->processFile(inputFile, outputFolder, framesTotal);
}
