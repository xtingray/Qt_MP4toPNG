#include <QObject>
#include <QDebug>

#ifdef __cplusplus
extern "C" {
// Libav libraries
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

// Required to create the PNG files
#include <png.h>
}
#endif

class QCutter : public QObject
{
    public:
        QCutter();
        ~QCutter();

        int processFile(const QString &videoFile, const QString &outputPath, int frames);

    private:
        // Decode packets into frames
        int decodePacket(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame);
        // Save a frame into a .png file
        int saveFrameToPng(AVFrame *frame, const QString &filename);

        QString filename;
        QString outputFolder;
        int imagesTotal;
};
