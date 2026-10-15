pragma Singleton
import QtQml
import org.kde.ki18n as KI18n

QtObject {
    id: root

    readonly property KI18n.KLocalizedQmlContext ctx: KI18n.KLocalizedQmlContext {
        translationDomain: "ghew"
    }

    function i18n(message, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10) {
        if (p10 !== undefined)
            return ctx.i18n(message, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
        if (p9 !== undefined)
            return ctx.i18n(message, p1, p2, p3, p4, p5, p6, p7, p8, p9)
        if (p8 !== undefined)
            return ctx.i18n(message, p1, p2, p3, p4, p5, p6, p7, p8)
        if (p7 !== undefined)
            return ctx.i18n(message, p1, p2, p3, p4, p5, p6, p7)
        if (p6 !== undefined)
            return ctx.i18n(message, p1, p2, p3, p4, p5, p6)
        if (p5 !== undefined)
            return ctx.i18n(message, p1, p2, p3, p4, p5)
        if (p4 !== undefined)
            return ctx.i18n(message, p1, p2, p3, p4)
        if (p3 !== undefined)
            return ctx.i18n(message, p1, p2, p3)
        if (p2 !== undefined)
            return ctx.i18n(message, p1, p2)
        if (p1 !== undefined)
            return ctx.i18n(message, p1)
        return ctx.i18n(message)
    }
}
