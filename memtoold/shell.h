/*
 * shell.h
 *
 *  Created on: 01.04.2010
 *      Author: chrschn
 */

#ifndef SHELL_H_
#define SHELL_H_

#include <QFile>
#include <QHash>
#include <QStringList>
#include <QTextStream>
#include <QThread>
#include <QVarLengthArray>
#include <QScriptValue>
#include <QSemaphore>
#include <QMutex>
#include "kernelsymbols.h"

// Forward declaration
class MemoryDump;
class QProcess;
class QScriptContext;
class QScriptEngine;
class QLocalServer;
class QLocalSocket;


/**
 * This class represents the interactive shell, which is the primary interface
 * for a user. It allows to load, save and parse the debugging symbols and show
 * various types of information.
 */
class Shell: public QThread
{
    Q_OBJECT

    /**
     * Generic call-back function for shell commands
     * @param args additional arguments passed to the command
     * @return returns zero in case of success, or a non-zero error code
     */
    typedef int (Shell::*ShellCallback)(QStringList);

    /**
     * Encapsulates a shell command that can be invoked from the command line
     */
    struct Command {
        /// Function to be executed when the command is invoked
        ShellCallback callback;
        /// Short help text for this command
        QString helpShort;
        /// Long help text for this command
        QString helpLong;

        /**
         * Constructor
         * @param call-back function to be called when the command is invoked
         * @param helpShort short help text
         * @param helpLong long help text
         */
        Command(ShellCallback callback = 0,
                const QString& helpShort = QString(),
                const QString& helpLong = QString())
            : callback(callback), helpShort(helpShort), helpLong(helpLong)
        {}
    };

public:
    /**
     * Constructor
     */
    Shell(bool listenOnSocket = false);

    /**
     * Destructor
     */
    ~Shell();

    /**
     * Use this stream to write information on the console.
     * @return the \c stdout stream of the shell
     */
    QTextStream& out();

    /**
     * Use this stream to write error messages on the console.
     * @return the \c stderr stream of the shell
     */
    QTextStream& err();

    /**
     * @return the KernelSymbol object of this Shell object
     */
    KernelSymbols& symbols();

    /**
     * @return the KernelSymbol object of this Shell object, const version
     */
    const KernelSymbols& symbols() const;

    /**
     * Reads a line of text from stdin and returns the reply.
     * @param prompt the text prompt the user sees
     * @return line of text from stdin
     */
    QString readLine(const QString& prompt = QString());

    /**
     * This flag indicates whether we are in an interactive session or not.
     * If non-interactive, the shell is less verbose (no progress information,
     * no prompts, no questions).
     * @return
     */
    bool interactive() const;

    /**
     * @return the exit code of the last executed command
     */
    int lastStatus() const;

    /**
     * Terminates the shell immediately
     */
    void shutdown();

protected:
    /**
     * Starts the interactive shell and does not return until the user invokes
     * the exit command.
     */
    virtual void run();

private slots:
    void pipeEndReadyReadStdOut();
    void pipeEndReadyReadStdErr();

    /**
     * Handles new connections to the local socket
     */
    void handleNewConnection();

    /**
     * Handles new data that is made available through the client socket
     */
    void handleSockReadyRead();

    /**
     * Handes disconnected() signals from the client socket
     */
    void handleSockDisconnected();

private:
    typedef QVarLengthArray<MemoryDump*, 16> MemDumpArray;

    static KernelSymbols _sym;
    QFile _stdin;
    QFile _stdout;
    QFile _stderr;
    QTextStream _out;
    QTextStream _err;
    QHash<QString, Command> _commands;
    static MemDumpArray _memDumps;
    QList<QProcess*> _pipedProcs;
    bool _listenOnSocket;
    bool _interactive;
    QLocalSocket* _clSocket;
    QLocalServer* _srvSocket;
    QSemaphore _sockSem;
    QMutex _sockSemLock;
    bool _finished;
    int _lastStatus;

    void cleanupPipedProcs();
    int eval(QString command);
    void hline(int width = 60);
    int parseMemDumpIndex(QStringList &args);
    static QScriptValue scriptListMemDumps(QScriptContext* ctx, QScriptEngine* eng);
    static QScriptValue scriptListVariableNames(QScriptContext* ctx, QScriptEngine* eng);
    static QScriptValue scriptListVariableIds(QScriptContext* ctx, QScriptEngine* eng);
    static QScriptValue scriptGetInstance(QScriptContext* ctx, QScriptEngine* eng);
    //---------------------------------
    int cmdExit(QStringList args);
    int cmdHelp(QStringList args);
    int cmdList(QStringList args);
    int cmdListSources(QStringList args);
    int cmdListTypes(QStringList args);
    int cmdListVars(QStringList args);
    int cmdListTypesById(QStringList args);
    int cmdListTypesByName(QStringList args);
    int cmdMemory(QStringList args);
    int cmdMemoryLoad(QStringList args);
    int cmdMemoryUnload(QStringList args);
    int cmdMemoryList(QStringList args);
    int cmdMemorySpecs(QStringList args);
    int cmdMemoryQuery(QStringList args);
    int cmdMemoryDump(QStringList args);
    int cmdScript(QStringList args);
    int cmdShow(QStringList args);
    int cmdShowBaseType(const BaseType* t);
    int cmdShowVariable(const Variable* v);
    int cmdSymbols(QStringList args);
    int cmdSymbolsParse(QStringList args);
    int cmdSymbolsLoad(QStringList args);
    int cmdSymbolsStore(QStringList args);
};

/// Globally accessible shell object
extern Shell* shell;

#endif /* SHELL_H_ */