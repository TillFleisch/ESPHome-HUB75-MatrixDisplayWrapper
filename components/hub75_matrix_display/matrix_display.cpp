#include "matrix_display.h"

namespace esphome
{
    namespace matrix_display
    {

        static const char *const TAG = "matrix_display";

        /**
         * Initialize the wrapped matrix display with user parameters
         */
        void MatrixDisplay::setup()
        {
            ESP_LOGCONFIG(TAG, "Setting up MatrixDisplay...");

            // Handle "never" and 0 to avoid divide by 0
            if (this->update_interval_ == 4294967295 || this->update_interval_ == 0) {
                // LVGL-driven: pick a constant
                this->mxconfig_.min_refresh_rate = 120;  // Hz
            } else {
                this->mxconfig_.min_refresh_rate = 1000 / this->update_interval_;
            }

            // Display Setup
            dma_display_ = new MatrixPanel_I2S_DMA(this->mxconfig_);
            this->dma_display_->begin();
            set_brightness(this->initial_brightness_);
            this->stored_brightness_ = this->initial_brightness_;
            this->dma_display_->clearScreen();

#ifdef USE_VIRTUAL_PANEL
            if (this->use_virtual_panel_) {
                // Wrap the physical DMA display with a virtual grid mapper
                this->virtual_display_ = new VPanelType(this->v_rows_, this->v_cols_,
                                                        this->mxconfig_.mx_width, this->mxconfig_.mx_height);
                this->virtual_display_->setDisplay(*this->dma_display_);
            }
#endif

            // Default to off if power switches are present
            set_state(!this->power_switches_.size());
        }

        /**
         * Updates the displayed image on the matrix. Dual buffers are used to prevent blanking in-between frames.
         */
        void MatrixDisplay::update()
        {
            if (!this->dma_display_) return;
            if (!this->enabled_) return;

            this->do_update_();

            // Flip buffer to show changes
            if (this->mxconfig_.double_buff) {
                this->dma_display_->flipDMABuffer();
            }
        }

        void MatrixDisplay::dump_config()
        {
            ESP_LOGCONFIG(TAG, "MatrixDisplay:");

            HUB75_I2S_CFG cfg = this->dma_display_->getCfg();

            // Log pin settings
            ESP_LOGCONFIG(TAG, "  Pins: R1:%i, G1:%i, B1:%i, R2:%i, G2:%i, B2:%i", cfg.gpio.r1, cfg.gpio.g1, cfg.gpio.b1, cfg.gpio.r2, cfg.gpio.g2, cfg.gpio.b2);
            ESP_LOGCONFIG(TAG, "  Pins: A:%i, B:%i, C:%i, D:%i, E:%i", cfg.gpio.a, cfg.gpio.b, cfg.gpio.c, cfg.gpio.d, cfg.gpio.e);
            ESP_LOGCONFIG(TAG, "  Pins: LAT:%i, OE:%i, CLK:%i", cfg.gpio.lat, cfg.gpio.oe, cfg.gpio.clk);

            // Log driver settings
            switch (cfg.driver)
            {
            case HUB75_I2S_CFG::shift_driver::SHIFTREG:
                ESP_LOGCONFIG(TAG, "  Driver: SHIFTREG");
                break;
            case HUB75_I2S_CFG::shift_driver::FM6124:
                ESP_LOGCONFIG(TAG, "  Driver: FM6124");
                break;
            case HUB75_I2S_CFG::shift_driver::FM6126A:
                ESP_LOGCONFIG(TAG, "  Driver: FM6126A");
                break;
            case HUB75_I2S_CFG::shift_driver::ICN2038S:
                ESP_LOGCONFIG(TAG, "  Driver: ICN2038S");
                break;
            case HUB75_I2S_CFG::shift_driver::MBI5124:
                ESP_LOGCONFIG(TAG, "  Driver: MBI5124");
                break;
            case HUB75_I2S_CFG::shift_driver::DP3246:
                ESP_LOGCONFIG(TAG, "  Driver: DP3246");
                break;
            }

            ESP_LOGCONFIG(TAG, "  I2S Speed: %u MHz", (uint32_t)cfg.i2sspeed / 1000000);
            ESP_LOGCONFIG(TAG, "  Latch Blanking: %i", cfg.latch_blanking);
            ESP_LOGCONFIG(TAG, "  Clock Phase: %s", TRUEFALSE(cfg.clkphase));
            ESP_LOGCONFIG(TAG, "  Min Refresh Rate: %i", cfg.min_refresh_rate);

#ifdef USE_VIRTUAL_PANEL
            ESP_LOGCONFIG(TAG, "  VirtualMatrix: %s", this->use_virtual_panel_ ? "ENABLED" : "disabled");
            if (this->use_virtual_panel_) {
                ESP_LOGCONFIG(TAG, "    Grid: %ux%u panels (total %u)",
                              (unsigned)this->v_cols_, (unsigned)this->v_rows_,
                              (unsigned)(this->v_cols_ * this->v_rows_));
            }
#endif
        }

        void MatrixDisplay::set_brightness(int brightness)
        {
            // Remember last non-zero so power-on can restore without fighting the number entity
            if (brightness > 0) this->stored_brightness_ = brightness;
            // Only apply to hardware when enabled; if off, just remember it
            if (this->enabled_ && this->dma_display_ != nullptr) {
                this->dma_display_->setBrightness8(brightness);
            }
        }

        void HOT MatrixDisplay::draw_absolute_pixel_internal(int x, int y, Color color)
        {
            if (!this->dma_display_) return;
            if (!this->enabled_) return;

            // Reject invalid pixels
            if (x >= this->get_width_internal() || x < 0 || y >= this->get_height_internal() || y < 0)
                return;

            // Update pixel value in buffer (route via virtual when enabled)
            this->draw_pixel_rgb888_(x, y, color.r, color.g, color.b);
        }

        inline uint8_t expand5to8(uint8_t v) { return (v << 3) | (v >> 2); }
        inline uint8_t expand6to8(uint8_t v) { return (v << 2) | (v >> 4); }

        void HOT MatrixDisplay::draw_pixels_at(
            int x_start, int y_start, int w, int h,
            const uint8_t *ptr,
            ColorOrder order,
            ColorBitness bitness,
            bool big_endian,
            int x_offset, int y_offset, int x_pad)
        {
            if (!this->dma_display_) return;
            if (!this->enabled_) return;

            const int stride_px = x_offset + w + x_pad;  // LVGL/ESPHome-provided stride.

            for (int yy = 0; yy < h; ++yy) {
                const int row_base_px = (y_offset + yy) * stride_px + x_offset;

                if (bitness == ColorBitness::COLOR_BITNESS_565) {
                    // Source is RGB565
                    const uint16_t *src16 = reinterpret_cast<const uint16_t *>(ptr);
                    for (int xx = 0; xx < w; ++xx) {
                        const uint8_t *p = reinterpret_cast<const uint8_t *>(&src16[row_base_px + xx]);
                        const uint16_t pix565 = big_endian ? (uint16_t(p[0]) << 8) | p[1] : (uint16_t(p[1]) << 8) | p[0];

                        uint8_t r = expand5to8((pix565 >> 11) & 0x1F);
                        uint8_t g = expand6to8((pix565 >> 5)  & 0x3F);
                        uint8_t b = expand5to8( pix565        & 0x1F);

                        this->draw_pixel_rgb888_(x_start + xx, y_start + yy, r, g, b);
                    }
                } else if (bitness == ColorBitness::COLOR_BITNESS_888) {
#if LV_COLOR_DEPTH == 32
                    // LVGL draw buffer is ARGB8888. Ignore alpha, honor endianness.
                    const uint8_t *src = ptr + row_base_px * 4;

                    for (int xx = 0; xx < w; ++xx) {
                        const uint8_t *p = src + xx * 4;

                        uint8_t r, g, b;
                        if (big_endian) {
                            // Big endian memory layout: [A][R][G][B]
                            r = p[1]; g = p[2]; b = p[3];
                        } else {
                            // Little endian (ESP32): [B][G][R][A]
                            r = p[2]; g = p[1]; b = p[0];
                        }

                        this->draw_pixel_rgb888_(x_start + xx, y_start + yy, r, g, b);
                    }
#else
                    // Should never happen with LVGL (no 24bpp mode). If we got here,
                    // color_bitness reporting is inconsistent with LV_COLOR_DEPTH.
                    ESP_LOGE(TAG, "Unexpected 888 bitness with LV_COLOR_DEPTH=%d", LV_COLOR_DEPTH);
#endif
                } else {
                    // error, not supported
                    ESP_LOGE(TAG, "Unexpected bitness=%d with LV_COLOR_DEPTH=%d", bitness, LV_COLOR_DEPTH);
                }
            }
        }

        void MatrixDisplay::fill(Color color)
        {
            if (!this->dma_display_) return;
            if (!this->enabled_) return;

            // Wrap fill screen method (route via virtual when enabled)
            this->fill_screen_rgb888_(color.r, color.g, color.b);
        }

        void MatrixDisplay::filled_rectangle(int x1, int y1, int width, int height, Color color)
        {
            if (!this->dma_display_) return;
            if (!this->enabled_) return;

#ifdef USE_VIRTUAL_PANEL
            if (this->use_virtual_panel_) {
                // Draw via per-pixel mapping when virtual wrapper is active
                for (int yy = 0; yy < height; ++yy) {
                    for (int xx = 0; xx < width; ++xx) {
                        this->draw_pixel_rgb888_(x1 + xx, y1 + yy, color.r, color.g, color.b);
                    }
                }
                return;
            }
#endif
            // Wrap fill rectangle method
            this->dma_display_->fillRect(x1, y1, width, height, color.r, color.g, color.b);
        }

    } // namespace matrix_display
} // namespace esphome
