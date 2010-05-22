LUPDATE = $$[QT_INSTALL_BINS]/lupdate
LRELEASE = $$[QT_INSTALL_BINS]/lrelease

contains(QT_VERSION, ^4\.[0-5]\..*):ts.commands = @echo This Qt version is too old for the ts target. Need Qt 4.6+.
else:ts.commands = $$LUPDATE . -ts $$TRANSLATIONS
QMAKE_EXTRA_TARGETS += ts

updateqm.input = TRANSLATIONS
updateqm.output = ${QMAKE_FILE_BASE}.qm
updateqm.commands = $$LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
updateqm.name = LRELEASE ${QMAKE_FILE_IN}
updateqm.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += updateqm
