
set( COMPONENT_PATH "../../components" )

set( COMPONENT_SRCS
    "./main.c"
    "./obd_mqtt_interface_explink.c"
    "../sdCard/sdCard.c"
    "../appOBD/source/obd_main.c"
    "../appOBD/source/simulated_route.c"
    "$ENV{IDF_PATH}/examples/common_components/protocol_examples_common/connect.c"
)

set ( COMPONENT_ADD_INCLUDEDIRS
    "./include"
    "../appOBD/include"
    "$ENV{IDF_PATH}/components/freertos/include/freertos"
)

register_component()
