set(PYTHON_SOURCES
    __init__.py
    bool_vector.py
    buffer.py
    ctime.py
    cstring.py
    double_vector.py
    hash.py
    int_vector.py
    install_abort_signals.py
    log.py
    lookup_table.py
    matrix.py
    profiler.py
    rng.py
    stat.py
    stringlist.py
    substitution_list.py
    thread_pool.py
    cthread_pool.py
    time_vector.py
    ui_return.py
    util_func.py
    vector_template.py
    version.py
    arg_pack.py
    path_format.py
)

add_python_package("python.ert.util"  ${PYTHON_INSTALL_PREFIX}/ert/util "${PYTHON_SOURCES}" True)

add_subdirectory(enums)
