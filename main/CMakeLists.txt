set(srcs "main.c"
        "gatt.c"
)

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS ".")
