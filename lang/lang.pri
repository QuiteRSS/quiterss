INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

TRANSLATIONS += lang/quiterss_de.ts lang/quiterss_ru.ts \
                lang/quiterss_es.ts lang/quiterss_fr.ts lang/quiterss_hu.ts \
                lang/quiterss_sv.ts lang/quiterss_sr.ts lang/quiterss_nl.ts \
                lang/quiterss_fa.ts lang/quiterss_it.ts lang/quiterss_zh_CN.ts \
                lang/quiterss_uk.ts lang/quiterss_cs.ts lang/quiterss_pl.ts \
                lang/quiterss_ja.ts lang/quiterss_ko.ts lang/quiterss_pt_BR.ts \
                lang/quiterss_lt.ts lang/quiterss_zh_TW.ts lang/quiterss_el_GR.ts \
                lang/quiterss_tr.ts lang/quiterss_ar.ts lang/quiterss_sk.ts \
                lang/quiterss_tg_TJ.ts lang/quiterss_pt_PT.ts lang/quiterss_vi.ts \
                lang/quiterss_ro_RO.ts lang/quiterss_fi.ts lang/quiterss_gl.ts \
                lang/quiterss_bg.ts lang/quiterss_hi.ts

isEmpty(QMAKE_LRELEASE) {
  Q_OS_WIN:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\lrelease.exe
  else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}

updateqm.input = TRANSLATIONS
updateqm.output = $$DESTDIR/lang/${QMAKE_FILE_BASE}.qm
updateqm.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm $$DESTDIR/lang/${QMAKE_FILE_BASE}.qm
updateqm.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += updateqm
