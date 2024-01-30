import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID

from ..matrix_display.display import MATRIX_ID, MatrixDisplay

AUTO_LOAD = ["switch"]

matrix_display_switch_ns = cg.esphome_ns.namespace(
    "matrix_display::matrix_display_switch"
)
MatrixDisplaySwitch = matrix_display_switch_ns.class_(
    "MatrixDisplaySwitch", switch.Switch, cg.Component
)

CONFIG_SCHEMA = switch.SWITCH_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(MatrixDisplaySwitch),
        cv.Required(MATRIX_ID): cv.use_id(MatrixDisplay),
    }
).extend(cv.COMPONENT_SCHEMA)


def to_code(config):
    matrix = yield cg.get_variable(config[MATRIX_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield switch.register_switch(var, config)

    cg.add(matrix.register_power_switch(var))
