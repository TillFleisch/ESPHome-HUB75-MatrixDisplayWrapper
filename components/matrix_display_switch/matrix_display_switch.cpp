#include "matrix_display_switch.h"

namespace esphome::matrix_display::matrix_display_switch
{
    void MatrixDisplaySwitch::write_state(bool state)
    {
        // Update display status and forwad state to all registered power switches.
        display_->set_state(state);
        for (MatrixDisplaySwitch *switch_ : display_->get_power_switches())
        {
            switch_->publish_state(state);
        }
    };

    void MatrixDisplaySwitch::dump_config()
    {
        ESP_LOGCONFIG(TAG, "MatrixDisplay Power Switch");
    };
} // namespace esphome::matrix_display::matrix_display_switch