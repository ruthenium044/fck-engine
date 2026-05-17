# This pattern is getting used because all modules are siblings
# That means the awkward add_subdiretory(../fckc fckc) becomes:
# find_package(fckc) - If we move them, we can account for it later!
add_subdirectory(../fckc fckc)