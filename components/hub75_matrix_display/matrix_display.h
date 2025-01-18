#pragma once

#include <utility>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/components/display/display_buffer.h"

#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"

namespace esphome
{
    namespace matrix_display
    {
        class MatrixDisplay;
        namespace matrix_display_switch
        {
            class MatrixDisplaySwitch;
            static void set_reference(MatrixDisplaySwitch *switch_, MatrixDisplay *display);
        } // namespace matrix_display_switch

        namespace matrix_display_brightness
        {
            class MatrixDisplayBrightness;
            static void set_reference(MatrixDisplayBrightness *brightness, MatrixDisplay *display);
        } // namespace matrix_display_brightness

        class MatrixDisplay : public display::DisplayBuffer
        {
        public:
            void setup() override;

            void dump_config() override;

            void update() override;

            /**
             * Registers a power switch on this matrix entity.
             *
             * @param power_switch Reference to a power switch
             */
            void register_power_switch(matrix_display_switch::MatrixDisplaySwitch *power_switch)
            {
                this->power_switches_.push_back(power_switch);
                set_reference(power_switch, this);
            };

            /**
             *Registers a brightness value on this matrix entity.
             *
             * @param brightness Reference to a brightness number entity
             */
            void register_brightness(matrix_display_brightness::MatrixDisplayBrightness *brightness)
            {
                this->brightness_values_.push_back(brightness);
                set_reference(brightness, this);
            };

            /**
             * Sets the hight of each individual panel.
             *
             * @param height height in pixels for a single panel
             */
            void set_panel_height(int panel_height)
            {
                this->mxconfig_.mx_height = panel_height;
            }

            /**
             * Sets the width of each individual panel.
             *
             * @param width width in pixels for a single panel
             */
            void set_panel_width(int panel_width)
            {
                this->mxconfig_.mx_width = panel_width;
            }

            /**
             * Sets the nr of panels chained on after another
             *
             * @param chain_length nr of panels
             */
            void set_chain_length(int chain_length)
            {
                this->mxconfig_.chain_length = chain_length;
            }

            /**
             * Sets the initial brightness of the display.
             *
             * @param brightness brightness value (0-255)
             */
            void set_initial_brightness(int brightness)
            {
                this->initial_brightness_ = brightness;
            };

            /**
             * Gets the inital brightness value from this display.
             */
            int get_initial_brightness()
            {
                return this->initial_brightness_;
            }

            void set_pins(InternalGPIOPin *R1_pin, InternalGPIOPin *G1_pin, InternalGPIOPin *B1_pin, InternalGPIOPin *R2_pin, InternalGPIOPin *G2_pin, InternalGPIOPin *B2_pin, InternalGPIOPin *A_pin, InternalGPIOPin *B_pin, InternalGPIOPin *C_pin, InternalGPIOPin *D_pin, InternalGPIOPin *E_pin, InternalGPIOPin *LAT_pin, InternalGPIOPin *OE_pin, InternalGPIOPin *CLK_pin)
            {
                this->mxconfig_.gpio = {
                    static_cast<int8_t>(R1_pin->get_pin()),
                    static_cast<int8_t>(G1_pin->get_pin()),
                    static_cast<int8_t>(B1_pin->get_pin()),
                    static_cast<int8_t>(R2_pin->get_pin()),
                    static_cast<int8_t>(G2_pin->get_pin()),
                    static_cast<int8_t>(B2_pin->get_pin()),
                    static_cast<int8_t>(A_pin->get_pin()),
                    static_cast<int8_t>(B_pin->get_pin()),
                    static_cast<int8_t>(C_pin->get_pin()),
                    static_cast<int8_t>(D_pin->get_pin()),
                    static_cast<int8_t>(E_pin != NULL ? E_pin->get_pin() : -1), // Set the e pin to -1 as required by the library if it is not used
                    static_cast<int8_t>(LAT_pin->get_pin()),
                    static_cast<int8_t>(OE_pin->get_pin()),
                    static_cast<int8_t>(CLK_pin->get_pin())};
            }

            /**
             * Sets the driver which will be used before display instantiation.
             *
             * @param driver Driver enum
             */
            void set_driver(HUB75_I2S_CFG::shift_driver driver)
            {
                this->mxconfig_.driver = driver;
            };

            /**
             * Sets the clock speed
             *
             * @param speed i2s clock speed
             */
            void set_i2sspeed(HUB75_I2S_CFG::clk_speed speed)
            {
                this->mxconfig_.i2sspeed = speed;
            };

            /**
             * Sets for how many cycles OE is blanked before and after LAT changes.
             *
             * @param latch_blanking cycle count
             */
            void set_latch_blanking(int latch_blanking)
            {
                this->mxconfig_.latch_blanking = latch_blanking;
            };

            /**
             * Sets the user defined clock phase and flag.
             *
             * @param clock_phase clock phase value
             */
            void set_clock_phase(bool clock_phase)
            {
                this->mxconfig_.clkphase = clock_phase;
            }

            display::DisplayType get_display_type() override
            {
                return display::DisplayType::DISPLAY_TYPE_COLOR;
            }

            /**
             * Fills the entire display with a given color.
             *
             * @param color Color used for filling the entire display
             */
            void fill(Color color) override;

            /**
             * Draws a filled rectangle on the display at the given location.
             *
             *
             * @param x1 x-coordinate of the rectangle
             * @param y1 y-coordinate of the rectangle
             * @param width width of the rectangle
             * @param height height of the rectangle
             * @param color Color used for filling the rectangle
             */
            void filled_rectangle(int x1, int y1, int width, int height, Color color = display::COLOR_ON);

            /**
             * Sets the on/off state of the matrix display
             *
             * @param state new state
             */
            void set_state(bool state)
            {
                this->enabled_ = state;
            }

            /**
             * Sets the brightness value of the display
             *
             * @param brightness new brightness value (0-255)
             */
            void set_brightness(int brightness);

            /**
             * Gets a vector of all registered power switches from this matrix display
             */
            std::vector<matrix_display_switch::MatrixDisplaySwitch *> get_power_switches()
            {
                return this->power_switches_;
            }

            /**
             * Gets a vector of all registered brightness number entities from this matrix display
             */
            std::vector<matrix_display_brightness::MatrixDisplayBrightness *> get_brightness_values()
            {
                return this->brightness_values_;
            }

        protected:
            /// @brief Wrapped matrix display
            MatrixPanel_I2S_DMA *dma_display_ = nullptr;

            /// @brief Matrix configuration
            HUB75_I2S_CFG mxconfig_;

            /// @brief initial brightness of the display
            int initial_brightness_ = 128;

            /// @brief on-off status of the display matrix
            bool enabled_ = false;

            /// @brief power switches belonging to this matrix display
            std::vector<matrix_display_switch::MatrixDisplaySwitch *> power_switches_;

            /// @brief brightness value number entities belonging to this matrix display
            std::vector<matrix_display_brightness::MatrixDisplayBrightness *> brightness_values_;

            int get_width_internal() override
            {
                return this->mxconfig_.mx_width * this->mxconfig_.chain_length;
            };
            int get_height_internal() override
            {
                return this->mxconfig_.mx_height;
            };

            /**
             * Draws a single pixel on the display.
             *
             * @param x x-coordinate of the pixel
             * @param y y-coordinate of the pixel
             * @param color Color of the pixel
             */
            void draw_absolute_pixel_internal(int x, int y, Color color) override;
        };

    } // namespace matrix_display
} // namespace esphome