#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "../matrix_display/matrix_display.h"

namespace esphome::matrix_display::matrix_display_switch
{
    static const char *TAG = "MatrixDisplaySwitch";

    class MatrixDisplaySwitch : public switch_::Switch, public Component
    {
    public:
        /**
         * Write the requested user state to hardware.
         */
        void write_state(bool state);

        void dump_config() override;

        /**
         * Sets the reference to the display component to which this switch belongs.
         *
         * @param display Matrix display component reference
         */
        void set_display(MatrixDisplay *display)
        {
            this->display_ = display;
        }

    protected:
        /// @brief display component to which this switch belongs
        MatrixDisplay *display_;
    };

    /**
     * Sets the reference to the display component to which this switch belongs.
     *
     * @param switch_ switch for which the reference should be set
     * @param display Matrix display component reference
     */
    static void set_reference(MatrixDisplaySwitch *switch_, MatrixDisplay *display)
    {
        switch_->set_display(display);
    }
} // namespace esphome::matrix_display::matrix_display_switch