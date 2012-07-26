# Set PREFIX, if not set
isEmpty(PREFIX):PREFIX = /usr/local

# Path for target
target.path += $$PREFIX/bin

# Extra target for scripts
scripts.path += $$PREFIX/share/insight/examples
scripts.files = scripts/*.js

# Extra target for tools
tools.path += $$PREFIX/share/insight/tools
tools.files = ../tools/gcc_pp \
	../tools/make-debug-kpkg \
	../tools/mount-img \
	../tools/umount-img

# What to install
INSTALLS += target scripts tools

# Should the memory_map feature be built? Disabled by default.
CONFIG += memory_map

CONFIG += console \
    debug_and_release

# Enable high optimization
QMAKE_CFLAGS_RELEASE += -O3
QMAKE_CXXFLAGS_RELEASE += -O3

QT += script \
    network
QT -= webkit

# Required libraries for building
LIBS += -lreadline \
    -L../libdebug \
    -ldebug \
    -L../libinsight \
    -linsight \
    -L../libantlr3c \
    -lantlr3c \
    -L../libcparser \
    -lcparser

# Inter-project include paths
INCLUDEPATH += ../libinsight/include \
    ../libdebug/include \
    ../libantlr3c/include \
    ../libcparser/include

SOURCES += kernelsourcetypeevaluator.cpp \
    kernelsourceparser.cpp \
    memorydumpsclass.cpp \
    scriptengine.cpp \
    kernelsymbolsclass.cpp \
    instancedata.cpp \
    instance.cpp \
    instanceprototype.cpp \
    instanceclass.cpp \
    memspecs.cpp \
    memspecparser.cpp \
    virtualmemory.cpp \
    memorydump.cpp \
    programoptions.cpp \
    longoperation.cpp \
    kernelsymbolparser.cpp \
    kernelsymbolwriter.cpp \
    kernelsymbolreader.cpp \
    volatiletype.cpp \
    funcpointer.cpp \
    refbasetype.cpp \
    typedef.cpp \
    consttype.cpp \
    symbol.cpp \
    array.cpp \
    compileunit.cpp \
    sourceref.cpp \
    shell.cpp \
    referencingtype.cpp \
    structuredmember.cpp \
    structured.cpp \
    typeinfo.cpp \
    symfactory.cpp \
    kernelsymbols.cpp \
    variable.cpp \
    pointer.cpp \
    numeric.cpp \
    main.cpp \
    basetype.cpp \
    enum.cpp \
    funcparam.cpp \
    function.cpp \
    shell_readline.cpp \
    astexpressionevaluator.cpp \
    astexpression.cpp \
    expressionresult.cpp \
    kernelsymbolstream.cpp \
    colorpalette.cpp \
    slubobjects.cpp \
    memorymapbuildersv.cpp \
    memorymapbuildercs.cpp \
    memorymapverifier.cpp \
    memorymapnodesv.cpp

HEADERS += kernelsourcetypeevaluator.h \
    kernelsourceparser.h \
    memorydumpsclass.h \
    scriptengine.h \
    kernelsymbolsclass.h \
    instance_def.h \
    priorityqueue.h \
    varsetter.h \
    instancedata.h \
    instance.h \
    instanceprototype.h \
    instanceclass.h \
    virtualmemoryexception.h \
    memspecs.h \
    memspecparser.h \
    virtualmemory.h \
    ioexception.h \
    filenotfoundexception.h \
    memorydump.h \
    programoptions.h \
    longoperation.h \
    parserexception.h \
    readerwriterexception.h \
    kernelsymbolconsts.h \
    kernelsymbolparser.h \
    kernelsymbolwriter.h \
    kernelsymbolreader.h \
    volatiletype.h \
    funcpointer.h \
    refbasetype.h \
    typedef.h \
    consttype.h \
    symbol.h \
    array.h \
    compileunit.h \
    sourceref.h \
    shell.h \
    referencingtype.h \
    structuredmember.h \
    structured.h \
    typeinfo.h \
    symfactory.h \
    kernelsymbols.h \
    variable.h \
    pointer.h \
    numeric.h \
    basetype.h \
    enum.h \
    funcparam.h \
    function.h \
    astexpressionevaluator.h \
    astexpression.h \
    expressionresult.h \
    kernelsymbolstream.h \
    colorpalette.h \
    slubobjects.h \
    memorymapbuildersv.h \
    memorymapbuildercs.h \
    memorymapverifier.h \
    memorymapnodesv.h

# Things to do when the memory map builder and widget is to be built. Enabling
# this feature requires InSight to run on an X server.
CONFIG(memory_map) {
    warning(Enabled compilation of the memory_map features. The resulting binary will must be run on an X windows system!)

    DEFINES += CONFIG_MEMORY_MAP
    QT += gui

    FORMS += memorymapwindow.ui

    SOURCES += memorydifftree.cpp \
        memorymaprangetree.cpp \
        memorymapbuilder.cpp \
        memorymapwindow.cpp \
        memorymapwidget.cpp \
        memorymapnode.cpp \
        memorymap.cpp

    HEADERS += memorydifftree.h \
        memorymaprangetree.h \
        memoryrangetree.h \
        memorymapbuilder.h \
        memorymapwindow.h \
        memorymapwidget.h \
        memorymapnode.h \
        memorymap.h
}
!CONFIG(memory_map) {
    QT -= gui
}

