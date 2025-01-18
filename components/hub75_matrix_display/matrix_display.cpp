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

            // Module configuration
            HUB75_I2S_CFG mxconfig(
                panel_width_,  // module width
                panel_height_, // module height
                chain_length_, // Chain length
                pins_);

            if (user_defined_driver_)
                mxconfig.driver = driver_;
            if (user_defined_i2sspeed_)
                mxconfig.i2sspeed = i2sspeed_;

            if (latch_blanking_ >= 0)
                mxconfig.latch_blanking = latch_blanking_;

            if (user_defined_clock_phase_)
                mxconfig.clkphase = clock_phase_;

            // The min refresh rate correlates with the update frequency of the component
            mxconfig.min_refresh_rate = 1000 / update_interval_;

            mxconfig.double_buff = true;

            // Display Setup
            dma_display_ = new MatrixPanel_I2S_DMA(mxconfig);
            dma_display_->begin();
            set_brightness(initial_brightness_);
            dma_display_->clearScreen();

            // Default to off if power switches are present
            set_state(!power_switches_.size());
        }

        /**
         * Updates the displayed image on the matrix. Dual buffers are used to prevent blanking in-between frames.
         */
        void MatrixDisplay::update()
        {
            if (enabled_)
            {
                // Draw updates to the screen
                this->do_update_();
            }
            else
            {
                dma_display_->clearScreen();
            }
            // Flip buffer to show changes
            dma_display_->flipDMABuffer();
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
            case HUB75_I2S_CFG::shift_driver::SM5266P:
                ESP_LOGCONFIG(TAG, "  Driver: SM5266P");
                break;
            case HUB75_I2S_CFG::shift_driver::DP3246_SM5368:
                ESP_LOGCONFIG(TAG, "  Driver: DP3246_SM5368");
                break;
            }

            ESP_LOGCONFIG(TAG, "  I2S Speed: %u MHz", (uint32_t)cfg.i2sspeed / 1000000);
            ESP_LOGCONFIG(TAG, "  Latch Blanking: %i", cfg.latch_blanking);
            ESP_LOGCONFIG(TAG, "  Clock Phase: %s", TRUEFALSE(cfg.clkphase));
            ESP_LOGCONFIG(TAG, "  Min Refresh Rate: %i", cfg.min_refresh_rate);
        }

        void MatrixDisplay::set_state(bool state)
        {
            enabled_ = state;
        }

        void MatrixDisplay::set_brightness(int brightness)
        {
            // Wrap brightness function
            dma_display_->setBrightness8(brightness);
        }

        void HOT MatrixDisplay::draw_absolute_pixel_internal(int x, int y, Color color)
        {
            // Reject invalid pixels
            if (x >= this->get_width_internal() || x < 0 || y >= this->get_height_internal() || y < 0)
                return;

            // Update pixel value in buffer
            dma_display_->drawPixelRGB888(x, y, color.r, color.g, color.b);
        }

        void MatrixDisplay::fill(Color color)
        {
            // Wrap fill screen method
            dma_display_->fillScreenRGB888(color.r, color.g, color.b);
        }

        void MatrixDisplay::filled_rectangle(int x1, int y1, int width, int height, Color color)
        {
            // Wrap fill rectangle method
            dma_display_->fillRect(x1, y1, width, width, color.r, color.g, color.b);
        }

    } // namespace matrix_display
} // namespace esphome
