# teleportation

build_lib_example(
    NAME telep-app-example
    SOURCE_FILES telep-app-example.cc
    LIBRARIES_TO_LINK ${libquantum}
)

build_lib_example(
    NAME telep-app-repeat-example
    SOURCE_FILES telep-app-repeat-example.cc
    LIBRARIES_TO_LINK ${libquantum}
)

build_lib_example(
    NAME msg-flow-example
    SOURCE_FILES msg-flow-example.cc
    LIBRARIES_TO_LINK ${libquantum}
)

build_lib_example(
    NAME telep-adapt-example
    SOURCE_FILES telep-adapt-example.cc
    LIBRARIES_TO_LINK ${libquantum}
)

build_lib_example(
    NAME telep-lin-example
    SOURCE_FILES telep-lin-example.cc
    LIBRARIES_TO_LINK ${libquantum}
)

build_lib_example(
    NAME telep-lin-adapt-example
    SOURCE_FILES telep-lin-adapt-example.cc
    LIBRARIES_TO_LINK ${libquantum}
)

# entanglement swapping

build_lib_example(
    NAME ent-swap-example
    SOURCE_FILES ent-swap-example.cc
    LIBRARIES_TO_LINK ${libquantum}
)

build_lib_example(
    NAME ent-swap-adapt-example
    SOURCE_FILES ent-swap-adapt-example.cc
    LIBRARIES_TO_LINK ${libquantum}
)

build_lib_example(
    NAME ent-swap-adapt-local-example
    SOURCE_FILES ent-swap-adapt-local-example.cc
    LIBRARIES_TO_LINK ${libquantum}
)

# distillation

build_lib_example(
    NAME distill-app-example
    SOURCE_FILES distill-app-example.cc
    LIBRARIES_TO_LINK ${libquantum}
)

build_lib_example(
    NAME distill-app-repeat-example
    SOURCE_FILES distill-app-repeat-example.cc
    LIBRARIES_TO_LINK ${libquantum}
)

build_lib_example(
    NAME distill-adapt-example
    SOURCE_FILES distill-adapt-example.cc
    LIBRARIES_TO_LINK ${libquantum}
)

build_lib_example(
    NAME distill-nested-example
    SOURCE_FILES distill-nested-example.cc
    LIBRARIES_TO_LINK ${libquantum}
)

build_lib_example(
    NAME distill-nested-adapt-example
    SOURCE_FILES distill-nested-adapt-example.cc
    LIBRARIES_TO_LINK ${libquantum}
)

