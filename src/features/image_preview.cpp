#include "features/image_preview.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <sstream>
#include <unistd.h>

#ifdef BUILD_IMAGE_PREVIEW_SUPPORT
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#endif

namespace fs = std::filesystem;

#ifdef BUILD_IMAGE_PREVIEW_SUPPORT
// RAII 包装类用于管理 FFmpeg 资源（在命名空间外部定义）
struct AVFormatContextDeleter {
    void operator()(AVFormatContext* ctx) {
        if (ctx) {
            avformat_close_input(&ctx);
        }
    }
};

struct AVCodecContextDeleter {
    void operator()(AVCodecContext* ctx) {
        if (ctx) {
            avcodec_free_context(&ctx);
        }
    }
};

struct AVFrameDeleter {
    void operator()(AVFrame* frame) {
        if (frame) {
            av_frame_free(&frame);
        }
    }
};

struct AVPacketDeleter {
    void operator()(AVPacket* packet) {
        if (packet) {
            av_packet_free(&packet);
        }
    }
};

struct SwsContextDeleter {
    void operator()(SwsContext* ctx) {
        if (ctx) {
            sws_freeContext(ctx);
        }
    }
};

struct AVBufferDeleter {
    void operator()(uint8_t* buf) {
        if (buf) {
            av_free(buf);
        }
    }
};

using AVFormatContextPtr = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>;
using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
using AVFramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;
using AVPacketPtr = std::unique_ptr<AVPacket, AVPacketDeleter>;
using SwsContextPtr = std::unique_ptr<SwsContext, SwsContextDeleter>;
using AVBufferPtr = std::unique_ptr<uint8_t, AVBufferDeleter>;
#endif

namespace pnana {
namespace features {

ImagePreview::ImagePreview()
    : loaded_(false), image_width_(0), image_height_(0), render_width_(0), render_height_(0) {}

ImagePreview::~ImagePreview() {
    clear();
}

bool ImagePreview::isImageFile(const std::string& filepath) {
    std::string ext = fs::path(filepath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif" || ext == ".bmp" ||
           ext == ".webp";
}

bool ImagePreview::isSupported() {
#ifdef BUILD_IMAGE_PREVIEW_SUPPORT
    return true;
#else
    return false;
#endif
}

bool ImagePreview::detectTrueColorSupport() {
    const char* term = getenv("TERM");
    const char* colorterm = getenv("COLORTERM");

    if (colorterm) {
        if (strstr(colorterm, "truecolor") || strstr(colorterm, "24bit")) {
            return true;
        }
    }

    if (term) {
        const char* truecolor_terms[] = {
            "xterm-256color", "screen-256color", "tmux-256color", "rxvt-unicode-256color",
            "alacritty",      "kitty",           "wezterm",       "vscode",
            "gnome-terminal", "konsole",         "terminator"};

        for (size_t i = 0; i < sizeof(truecolor_terms) / sizeof(truecolor_terms[0]); i++) {
            if (strstr(term, truecolor_terms[i])) {
                return true;
            }
        }
    }

    if (isatty(STDOUT_FILENO)) {
        return true; // 大多数现代终端都支持
    }

    return false;
}

unsigned char ImagePreview::rgbToGray(unsigned char r, unsigned char g, unsigned char b) {
    return static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);
}

std::string ImagePreview::getCharForGray(unsigned char gray_value) {
    // 使用 Unicode 块状字符
    int index = (gray_value * 3) / 255;
    if (index < 0)
        index = 0;
    if (index > 3)
        index = 3;

    const char* unicode_chars[] = {"░", "▒", "▓", "█"};
    return std::string(unicode_chars[index]);
}

std::string ImagePreview::getColorCode(unsigned char r, unsigned char g, unsigned char b) {
    if (detectTrueColorSupport()) {
        // 24位真彩色
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "\033[38;2;%d;%d;%dm", r, g, b);
        return std::string(buffer);
    } else {
        // 256色模式（简化版）
        int gray = (r + g + b) / 3;
        int color_code = 232 + (gray * 23) / 255;
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "\033[38;5;%dm", color_code);
        return std::string(buffer);
    }
}

bool ImagePreview::loadImage(const std::string& filepath, int width, int max_height) {
    clear();

#ifdef BUILD_IMAGE_PREVIEW_SUPPORT
    if (!fs::exists(filepath) || !fs::is_regular_file(filepath)) {
        return false;
    }

    // 设置一个合理的上限，避免处理超大图片导致性能问题
    const int MAX_PREVIEW_WIDTH = 300;  // 最大宽度限制
    const int MAX_PREVIEW_HEIGHT = 150; // 最大高度限制
    if (width > MAX_PREVIEW_WIDTH) {
        width = MAX_PREVIEW_WIDTH;
    }
    if (max_height > 0 && max_height > MAX_PREVIEW_HEIGHT) {
        max_height = MAX_PREVIEW_HEIGHT;
    }

    // 初始化 FFmpeg（使用 RAII 确保清理）
    class FFmpegInit {
      public:
        FFmpegInit() {
            avformat_network_init();
        }
        ~FFmpegInit() {
            avformat_network_deinit();
        }
    };
    FFmpegInit ffmpeg_init;

    // 使用 RAII 管理 FFmpeg 资源
    AVFormatContext* format_ctx_raw = nullptr;
    int ret = avformat_open_input(&format_ctx_raw, filepath.c_str(), nullptr, nullptr);
    if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_ERROR("ImagePreview::loadImage() - Failed to open file: " + std::string(errbuf));
        return false;
    }
    AVFormatContextPtr format_ctx(format_ctx_raw);

    // 查找流信息
    ret = avformat_find_stream_info(format_ctx.get(), nullptr);
    if (ret < 0) {
        LOG_ERROR("ImagePreview::loadImage() - Failed to find stream info");
        return false;
    }

    // 查找视频流（图片在 FFmpeg 中被视为单帧视频）
    int video_stream_index = -1;
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        LOG_ERROR("ImagePreview::loadImage() - No video stream found");
        return false;
    }

    // 获取解码器参数
    AVCodecParameters* codecpar = format_ctx->streams[video_stream_index]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        LOG_ERROR("ImagePreview::loadImage() - Codec not found");
        return false;
    }

    // 创建解码器上下文（RAII）
    AVCodecContext* codec_ctx_raw = avcodec_alloc_context3(codec);
    if (!codec_ctx_raw) {
        LOG_ERROR("ImagePreview::loadImage() - Failed to allocate codec context");
        return false;
    }
    AVCodecContextPtr codec_ctx(codec_ctx_raw);

    ret = avcodec_parameters_to_context(codec_ctx.get(), codecpar);
    if (ret < 0) {
        LOG_ERROR("ImagePreview::loadImage() - Failed to copy codec parameters");
        return false;
    }

    // 打开解码器
    ret = avcodec_open2(codec_ctx.get(), codec, nullptr);
    if (ret < 0) {
        LOG_ERROR("ImagePreview::loadImage() - Failed to open codec");
        return false;
    }

    int x = codec_ctx->width;
    int y = codec_ctx->height;

    image_width_ = x;
    image_height_ = y;
    image_path_ = filepath;

    // 分配帧（RAII）
    AVFramePtr frame(av_frame_alloc());
    AVFramePtr rgb_frame(av_frame_alloc());
    AVPacketPtr packet(av_packet_alloc());

    if (!frame || !rgb_frame || !packet) {
        LOG_ERROR("ImagePreview::loadImage() - Failed to allocate frames");
        return false;
    }

    // 读取并解码帧
    ret = av_read_frame(format_ctx.get(), packet.get());
    if (ret < 0) {
        LOG_ERROR("ImagePreview::loadImage() - Failed to read frame");
        return false;
    }

    if (packet->stream_index == video_stream_index) {
        ret = avcodec_send_packet(codec_ctx.get(), packet.get());
        if (ret < 0) {
            LOG_ERROR("ImagePreview::loadImage() - Failed to send packet");
            return false;
        }

        ret = avcodec_receive_frame(codec_ctx.get(), frame.get());
        if (ret < 0) {
            LOG_ERROR("ImagePreview::loadImage() - Failed to receive frame");
            return false;
        }

        // 创建图像转换上下文（RAII）
        SwsContextPtr sws_ctx(sws_getContext(x, y, codec_ctx->pix_fmt, x, y, AV_PIX_FMT_RGB24,
                                             SWS_BILINEAR, nullptr, nullptr, nullptr));

        if (!sws_ctx) {
            LOG_ERROR("ImagePreview::loadImage() - Failed to create sws context");
            return false;
        }

        // 分配 RGB 缓冲区（RAII）
        int rgb_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, x, y, 1);
        AVBufferPtr rgb_buffer(static_cast<uint8_t*>(av_malloc(rgb_buffer_size)));
        if (!rgb_buffer) {
            LOG_ERROR("ImagePreview::loadImage() - Failed to allocate RGB buffer");
            return false;
        }

        av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, rgb_buffer.get(),
                             AV_PIX_FMT_RGB24, x, y, 1);

        // 转换格式
        sws_scale(sws_ctx.get(), frame->data, frame->linesize, 0, y, rgb_frame->data,
                  rgb_frame->linesize);

        // 现在 rgb_frame->data[0] 包含 RGB24 格式的数据
        uint8_t* rgb_data = rgb_frame->data[0];

        // 计算缩放比例，根据代码区尺寸精确计算，确保不截断
        float scale = static_cast<float>(width) / x;
        int new_height = static_cast<int>(y * scale * 0.6f); // 字符高度约为宽度的0.6倍

        // 如果指定了最大高度，确保不超过
        if (max_height > 0 && new_height > max_height) {
            // 根据最大高度重新计算宽度和缩放比例
            new_height = max_height;
            scale = static_cast<float>(new_height) / (y * 0.6f);
            width = static_cast<int>(x * scale);
            // 确保宽度不超过原始请求的宽度
            if (width > MAX_PREVIEW_WIDTH) {
                width = MAX_PREVIEW_WIDTH;
                scale = static_cast<float>(width) / x;
                new_height = static_cast<int>(y * scale * 0.6f);
            }
        } else if (new_height > MAX_PREVIEW_HEIGHT) {
            new_height = MAX_PREVIEW_HEIGHT;
            scale = static_cast<float>(new_height) / (y * 0.6f);
            width = static_cast<int>(x * scale);
        }

        if (new_height <= 0)
            new_height = 1;
        if (width <= 0)
            width = 1;

        render_width_ = width;
        render_height_ = new_height;

        bool use_color = true;

        // 生成 ASCII 艺术
        preview_lines_.clear();
        preview_pixels_.clear();
        preview_pixels_.resize(new_height);

        for (int i = 0; i < new_height; i++) {
            std::string line;
            preview_pixels_[i].resize(width);

            for (int j = 0; j < width; j++) {
                // 计算原始图片中的对应位置
                float orig_x_f = static_cast<float>(j) / scale;
                float orig_y_f = static_cast<float>(i) / scale / 0.6f;

                int orig_x = static_cast<int>(orig_x_f);
                int orig_y = static_cast<int>(orig_y_f);

                if (orig_x >= x)
                    orig_x = x - 1;
                if (orig_y >= y)
                    orig_y = y - 1;
                if (orig_x < 0)
                    orig_x = 0;
                if (orig_y < 0)
                    orig_y = 0;

                // 从 RGB 数据中读取像素（RGB24 格式：每像素3字节，按行存储）
                int pixel_offset = (orig_y * rgb_frame->linesize[0]) + (orig_x * 3);
                if (pixel_offset >= 0 && pixel_offset < rgb_buffer_size - 2) {
                    unsigned char r = rgb_data[pixel_offset];
                    unsigned char g = rgb_data[pixel_offset + 1];
                    unsigned char b = rgb_data[pixel_offset + 2];

                    unsigned char gray = rgbToGray(r, g, b);
                    std::string char_str = getCharForGray(gray);

                    // 保存像素数据
                    PreviewPixel pixel;
                    pixel.r = r;
                    pixel.g = g;
                    pixel.b = b;
                    pixel.ch = char_str;
                    preview_pixels_[i][j] = pixel;

                    if (use_color) {
                        std::string color_code = getColorCode(r, g, b);
                        line += color_code + char_str + "\033[0m";
                    } else {
                        line += char_str;
                    }
                } else {
                    line += " ";
                    PreviewPixel pixel;
                    pixel.r = pixel.g = pixel.b = 0;
                    pixel.ch = " ";
                    preview_pixels_[i][j] = pixel;
                }
            }

            preview_lines_.push_back(line);
        }

        loaded_ = true;
        return true;
    }

    return false;
#else
    // FFmpeg 未安装，图片预览功能被禁用
    LOG_ERROR("ImagePreview::loadImage() - FFmpeg not available, image preview disabled");
    return false;
#endif
}

void ImagePreview::clear() {
    preview_lines_.clear();
    preview_pixels_.clear();
    loaded_ = false;
    image_width_ = 0;
    image_height_ = 0;
    image_path_.clear();
    render_width_ = 0;
    render_height_ = 0;
}

} // namespace features
} // namespace pnana
