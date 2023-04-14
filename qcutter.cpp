#include "qcutter.h" 

QCutter::QCutter()
{
}

QCutter::~QCutter()
{
}

int QCutter::processFile(const QString &videoFile, const QString &outputPath, int frames)
{
    outputFolder = outputPath;
    imagesTotal = frames;
    qDebug() << "[QCutter::processFile()] - Initializing all the containers, codecs and protocols...";

    // AVFormatContext holds the header information from the format (Container)
    // Allocating memory for this component
    // http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html
    AVFormatContext *formatContext = avformat_alloc_context();
    if (!formatContext) {
        qWarning() << "[QCutter::processFile()] - ERROR could not allocate memory for Format Context";

        return -1;
    }

    qDebug() << "[QCutter::processFile()] - Opening the input file (%s) and loading format (container) header -> " << videoFile;
    // Open the file and read its header. The codecs are not opened.
    // The function arguments are:
    // AVFormatContext (the component we allocated memory for),
    // url (filename),
    // AVInputFormat (if you pass nullptr it'll do the auto detect)
    // and AVDictionary (which are options to the demuxer)
    // http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html
    QByteArray bytes = videoFile.toLocal8Bit();
    const char *inputFile = bytes.data();

    if (avformat_open_input(&formatContext, inputFile, nullptr, nullptr) != 0) {
        qWarning() << "[QCutter::processFile()] - ERROR could not open the file";

        return -1;
    }

    // now we have access to some information about our file
    // since we read its header we can say what format (container) it's
    // and some other information related to the format itself.
    qDebug() << "[QCutter::processFile()] - Format: " << formatContext->iformat->name
             << ", Duration: " << formatContext->duration
             << ", Bitrate: " << formatContext->bit_rate;

    qDebug() << "[QCutter::processFile()] - Finding stream info from format...";
    // read Packets from the Format to get stream information
    // this function populates pFormatContext->streams
    // (of size equals to pFormatContext->nb_streams)
    // the arguments are:
    // the AVFormatContext
    // and options contains options for codec corresponding to i-th stream.
    // On return each dictionary will be filled with options that were not found.
    // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        qWarning() << "[QCutter::processFile()] - ERROR could not get the stream info";

        return -1;
    }

    // The component that knows how to enCOde and DECode the stream
    // it's the codec (audio or video)
    // http://ffmpeg.org/doxygen/trunk/structAVCodec.html
    AVCodec *inputCodec = nullptr;
    // this component describes the properties of a codec used by the stream i
    // https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html
    AVCodecParameters *inputCodecParameters = nullptr;
    int videoStreamIndex = -1;

    // Loop though all the streams and print its main information
    int streamsTotal = (int) formatContext->nb_streams;
    for (int i = 0; i < streamsTotal; i++) {
        AVCodecParameters *codecParameters = nullptr;
        codecParameters = formatContext->streams[i]->codecpar;
        qDebug() << "[QCutter::processFile()] - AVStream->time_base before open coded -> "
                 << formatContext->streams[i]->time_base.num << "/" << formatContext->streams[i]->time_base.den;
        qDebug() << "[QCutter::processFile()] - AVStream->r_frame_rate before open coded -> "
                 << formatContext->streams[i]->r_frame_rate.num << "/" << formatContext->streams[i]->r_frame_rate.den;
        qDebug() << "[QCutter::processFile()] - AVStream->start_time -> " << formatContext->streams[i]->start_time;
        qDebug() << "[QCutter::processFile()] - AVStream->duration -> " << formatContext->streams[i]->duration;
        qDebug() << "[QCutter::processFile()] - Finding the proper decoder (CODEC)";
        qDebug() << "---";

        AVCodec *codec = nullptr;

        // Finds the registered decoder for a codec ID
        // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html
        codec = avcodec_find_decoder(codecParameters->codec_id);

        if (codec == nullptr) {
            qWarning() << "[QCutter::processFile()] - ERROR unsupported codec!";
            // In this example if the codec is not found we just skip it
            continue;
        }

        // When the stream is a video we store its index, codec parameters and codec
        if (codecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (videoStreamIndex == -1) {
                videoStreamIndex = i;
                inputCodec = codec;
                inputCodecParameters = codecParameters;
            }

            qDebug() << "[QCutter::processFile()] - Video Codec: resolution -> " << codecParameters->width << " x " << codecParameters->height;
        } else if (codecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            qDebug() << "[QCutter::processFile()] -  Audio Codec -> " << codecParameters->channels
                     << " channels, sample rate -> " << codecParameters->sample_rate;
        }

        // Print its name, id and bitrate
        qDebug() << "[QCutter::processFile()] - Codec -> " << codec->name << ", ID " << codec->id << ", bit_rate " << codecParameters->bit_rate;
    }

    if (videoStreamIndex == -1) {
        qWarning() << "[QCutter::processFile()] - File %s does not contain a video stream! -> " << videoFile;

        return -1;
    }

    // https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
    AVCodecContext *inputCodecContext = avcodec_alloc_context3(inputCodec);
    if (!inputCodecContext) {
        qWarning() << "[QCutter::processFile()] - Failed to allocated memory for AVCodecContext";

        return -1;
    }

    // Fill the codec context based on the values from the supplied codec parameters
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html
    if (avcodec_parameters_to_context(inputCodecContext, inputCodecParameters) < 0) {
       qWarning() << "[QCutter::processFile()] - Failed to copy codec params to codec context";

       return -1;
    }

    // Initialize the AVCodecContext to use the given AVCodec.
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html
    if (avcodec_open2(inputCodecContext, inputCodec, nullptr) < 0) {
        qWarning() << "[QCutter::processFile()] - Failed to open codec through avcodec_open2";

        return -1;
    }

    // https://ffmpeg.org/doxygen/trunk/structAVFrame.html
    AVFrame *inputFrame = av_frame_alloc();
    if (!inputFrame) {
        qWarning() << "[QCutter::processFile()] - Failed to allocate memory for AVFrame";

        return -1;
    }

    // https://ffmpeg.org/doxygen/trunk/structAVPacket.html
    AVPacket *inputPacket = av_packet_alloc();
    if (!inputPacket) {
        qWarning() << "[QCutter::processFile()] - Failed to allocate memory for AVPacket";

        return -1;
    }

    int ret = 0;
    int counter = 0;

    // Fill the Packet with data from the Stream
    // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html
    while (av_read_frame(formatContext, inputPacket) >= 0) {
        // If it's the video stream
        if (inputPacket->stream_index == videoStreamIndex) {
            qDebug() << "---";
            qDebug() << "[QCutter::processFile()]    - AVPacket->pts -> " << inputPacket->pts;
            ret = decodePacket(inputPacket, inputCodecContext, inputFrame);
            if (ret < 0)
                break;
            // Stop it, otherwise we'll be saving hundreds of frames
            if (counter > imagesTotal)
                break;
            counter++;
        }
        // https://ffmpeg.org/doxygen/trunk/group__lavc__packet.html
        av_packet_unref(inputPacket);
    }

    qDebug() << "---";
    qDebug() << "[QCutter::processFile()] - Releasing all the resources...";

    avformat_close_input(&formatContext);
    av_packet_free(&inputPacket);
    av_frame_free(&inputFrame);
    avcodec_free_context(&inputCodecContext);

    return 0;
}

int QCutter::decodePacket(AVPacket *pPacket, AVCodecContext *codecContext, AVFrame *frame)
{
    // Supply raw packet data as input to a decoder
    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html
    int ret = avcodec_send_packet(codecContext, pPacket);

    if (ret < 0) {
        qWarning() << "[QCutter::decodePacket()] - Error while sending a packet to the decoder -> " << ret;

        return ret;
    }

    while (ret >= 0) {
        // Return decoded output data (into a frame) from a decoder
        // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html
        ret = avcodec_receive_frame(codecContext, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            qWarning() << "[QCutter::decodePacket()] - Error while receiving a frame from the decoder -> " << ret;

            return ret;
        }

        if (ret >= 0) {
            qDebug() << "[QCutter::decodePacket()]   - Frame -> " << codecContext->frame_number
                     << " (type=" << av_get_picture_type_char(frame->pict_type) << ", size=" << frame->pkt_size
                     << " bytes, format=" << frame->format << ") pts " << frame->pts
                     << " key_frame " << frame->key_frame
                     << " [DTS " "]" << frame->coded_picture_number;

            QString frameFilename = outputFolder + "/frame" + QString::number(codecContext->frame_number) + ".png";

            // Check if the frame is a planar YUV 4:2:0, 12bpp
            // That is the format of the provided .mp4 file
            // RGB formats will definitely not give a gray image
            // Other YUV image may do so, but untested, so give a warning
            if (frame->format != AV_PIX_FMT_YUV420P) {
                qWarning() << "[QCutter::decodePacket()] - "
                            "Warning: the generated file may not be a grayscale image, "
                            "but could e.g. be just the R component if the video format is RGB";
            }

            // To create the PNG files, the AVFrame data must be translated from YUV420P format into RGB24
            struct SwsContext *swsContext = sws_getContext(
                frame->width, frame->height, AV_PIX_FMT_YUV420P,
                frame->width, frame->height, AV_PIX_FMT_RGB24,
                SWS_BILINEAR, nullptr, nullptr, nullptr);

            // Allocate a new AVFrame for the output RGB24 image
            AVFrame* rgbFrame = av_frame_alloc();

            // Set the properties of the output AVFrame
            rgbFrame->format = AV_PIX_FMT_RGB24;
            rgbFrame->width = frame->width;
            rgbFrame->height = frame->height;

            int ret = av_frame_get_buffer(rgbFrame, 0);
            if (ret < 0) {
                qWarning() << "[QCutter::decodePacket()] - Error while preparing RGB frame -> " << ret;

                return ret;
            }

            qDebug() << "[QCutter::decodePacket()]   - Transforming frame format from YUV420P into RGB24...";
            ret = sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, rgbFrame->data, rgbFrame->linesize);
            if (ret < 0) {
                qWarning() << "[QCutter::decodePacket()] - Error while translating the frame format from YUV420P into RGB24 -> " << ret;

                return ret;
            }

            // Save a frame into a .PNG file
            ret = saveFrameToPng(rgbFrame, frameFilename);
            if (ret < 0) {
                qWarning() << "[QCutter::decodePacket()] - Failed to write PNG file";

                return -1;
            }

            av_frame_free(&rgbFrame);
        }
    }

    return 0;
}

// Function to save an AVFrame to a PNG file
int QCutter::saveFrameToPng(AVFrame *frame, const QString &outputFilename)
{
    int ret = 0;

    qDebug() << "[QCutter::saveFrameToPng()] - Creating PNG file -> " << outputFilename;

    QByteArray bytes = outputFilename.toLocal8Bit();
    const char *pngFile = bytes.data();
    // Open the PNG file for writing
    FILE *fp = fopen(pngFile, "wb");
    if (!fp) {
        qWarning() << "[QCutter::saveFrameToPng()] - Failed to open file -> " << outputFilename;

        return -1;
    }

    // Create the PNG write struct and info struct
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr) {
        qWarning() << "[QCutter::saveFrameToPng()] - Failed to create PNG write struct";
        fclose(fp);

        return -1;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        qWarning() << "[QCutter::saveFrameToPng()] - Failed to create PNG info struct";
        png_destroy_write_struct(&png_ptr, nullptr);
        fclose(fp);

        return -1;
    }

    // Set up error handling for libpng
    if (setjmp(png_jmpbuf(png_ptr))) {
        qWarning() << "[QCutter::saveFrameToPng()] - Error writing PNG file";
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);

        return -1;
    }

    // Set the PNG file as the output for libpng
    png_init_io(png_ptr, fp);

    // Set the PNG image attributes
    png_set_IHDR(png_ptr, info_ptr, frame->width, frame->height, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    // Allocate memory for the row pointers and fill them with the AVFrame data
    png_bytep *row_pointers = (png_bytep *) malloc(sizeof(png_bytep) * frame->height);
    for (int y = 0; y < frame->height; y++) {
        row_pointers[y] = (png_bytep) (frame->data[0] + y * frame->linesize[0]);
    }

    // Write the PNG file
    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

    // Clean up
    free(row_pointers);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);

    return ret;
}
