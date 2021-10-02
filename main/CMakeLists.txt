
set(component_srcs "main.c")

# set(include_dirs ".")

# $(PROJECT_PATH)

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include")
